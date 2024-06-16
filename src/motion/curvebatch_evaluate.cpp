#include <cstdint>
#include <immintrin.h>

#define USE_SHUFFLE

union SReg {
  __m128 f;
  __m128i i;
};

union AReg {
  __m256 f;
  __m256i i;
};

union VReg {
  __m512 f;
  __m512i i;
};

union CacheLine {
  SReg s[4];
  AReg a[2];
  VReg v;
};

#ifdef USE_SHUFFLE
void Decompose4x32b(__m128i input, CacheLine &out) {
  out.s[0].i = _mm_shuffle_epi32(input, 0);
  out.s[1].i = _mm_shuffle_epi32(input, 0b0101'0101);
  out.s[2].i = _mm_shuffle_epi32(input, 0b1010'1010);
  out.s[3].i = _mm_shuffle_epi32(input, 0xff);
}
#else
void Decompose4x32b(SReg input, CacheLine &out) {
  out.s[0].f = _mm_moveldup_ps(input.f);
  out.s[2].f = _mm_movehdup_ps(input.f);
  out.s[1].f = _mm_movehdup_ps(out[0].f);
  out.s[3].f = _mm_movehdup_ps(out[2].f);
  out.s[0].f = _mm_moveldup_ps(out[0].f);
  out.s[2].f = _mm_moveldup_ps(out[2].f);
}
#endif

#ifdef __AVX2__
void Decompose8x32b(AReg input, CacheLine out[2]) {
  out[0].a[0].i = _mm256_shuffle_epi32(input.i, 0);
  out[0].a[1].i = _mm256_shuffle_epi32(input.i, 0b0101'0101);
  out[1].a[0].i = _mm256_shuffle_epi32(input.i, 0b1010'1010);
  out[1].a[1].i = _mm256_shuffle_epi32(input.i, 0xff);
}

void Decompose16x16b(__m128i input[2], CacheLine &out) {
  out.a[0].i = _mm256_cvtepu16_epi32(input[0]);
  out.a[1].i = _mm256_cvtepu16_epi32(input[1]);
}

void Decompose4x16bLow(CacheLine input, CacheLine out[2]) {
  __m256i line0 = _mm256_cvtepu16_epi32(input.s[0].i);
  __m256i line1 = _mm256_cvtepu16_epi32(input.s[3].i);
  line1 = _mm256_slli_epi32(line1, 16);

  AReg lineo;
  lineo.i = _mm256_or_si256(line0, line1);

  Decompose8x32b(lineo, out);
}

void Decompose4x16bMid(CacheLine input, CacheLine out[2]) {
  __m256i line0 = _mm256_cvtepu16_epi32(input.s[1].i);
  __m256i line1 = _mm256_cvtepu16_epi32(input.s[3].i);
  line1 = _mm256_slli_epi32(line1, 16);

  AReg lineo;
  lineo.i = _mm256_or_si256(line0, line1);

  Decompose8x32b(lineo, out);
}

void Decompose4x16bHigh(CacheLine input, CacheLine out[2]) {
  __m256i line0 = _mm256_cvtepu16_epi32(input.s[2].i);
  __m256i line1 = _mm256_cvtepu16_epi32(input.s[3].i);
  line1 = _mm256_slli_epi32(line1, 16);

  AReg lineo;
  lineo.i = _mm256_or_si256(line0, line1);

  Decompose8x32b(lineo, out);
}

constexpr float F0 = 0x8;
constexpr float F1 = 0x80000000 * (1.f / (1U << 31) * 2.f);

void ConvertCacheline4x4(CacheLine line, CacheLine scale, CacheLine offset) {
  for (int i = 0; i < 2; i++) {
    line.a[i].f = _mm256_fmadd_ps(_mm256_cvtepi32_ps(line.a[i].i), scale.a[i].f,
                                  offset.a[i].f);
  }
}

void ConvertCacheline4x4(CacheLine line, CacheLine mask, CacheLine scale,
                         CacheLine offset) {
  for (int i = 0; i < 2; i++) {
    line.a[i].i = _mm256_and_si256(line.a[i].i, mask.a[i].i);
    //_mm256_srli_epi32(_mm256_mul_epu32(line.a[i].i, mask.a[i].i), 16);
    line.a[i].f = _mm256_mul_ps(_mm256_cvtepi32_ps(line.a[i].i), scale.a[i].f);
    line.a[i].f = _mm256_add_ps(line.a[i].f, offset.a[i].f);
  }
}

// Compute W component (4th) for 4 packed quaternions
// sqrt(1 - q.xyz . q.xyz)
void ComputeQuatReal(CacheLine &line) {
  __m256 ldp = _mm256_dp_ps(line.a[0].f, line.a[0].f, 0b111'0101);
  __m256 hdp = _mm256_dp_ps(line.a[1].f, line.a[1].f, 0b111'1010);
  __m256 mdp = _mm256_or_ps(ldp, hdp);

  __m256 linelSub = _mm256_sub_ps(_mm256_set1_ps(1), mdp);
  __m256 result = _mm256_sqrt_ps(linelSub);
  __m256 sdpl = _mm256_permutevar8x32_ps(
      result, _mm256_set_epi32(6, 0, 0, 0, 0, 0, 0, 0));
  __m256 sdph = _mm256_permutevar8x32_ps(
      result, _mm256_set_epi32(7, 0, 0, 0, 1, 0, 0, 0));
  line.a[0].f = _mm256_blend_ps(line.a[0].f, sdpl, 0x88);
  line.a[1].f = _mm256_blend_ps(line.a[1].f, sdph, 0x88);
}

__m128 QuatMult(__m256 parent, __m256 quat) {
  __m256i ymm0Mask{_mm256_set_epi32(0, 2, 1, 0, 3, 3, 3, 3)};
  __m256i ymm2Mask{_mm256_set_epi32(2, 1, 0, 2, 1, 0, 2, 1)};
  // ymm0
  // parent: w, w, w, w
  // parent: x, y, z, x
  __m256 ymm0 = _mm256_permutevar8x32_ps(parent, ymm0Mask);

  // ymm2
  // parent: y, z, x, y
  // parent: z, x, y, z
  __m256 ymm2 = _mm256_permutevar8x32_ps(parent, ymm2Mask);

  __m256i ymm1Mask{_mm256_set_epi32(0, 3, 3, 3, 3, 2, 1, 0)};
  __m256i ymm3Mask{_mm256_set_epi32(2, 0, 2, 1, 1, 1, 0, 2)};
  // ymm1
  // quater: x, y, z, w
  // quater: w, w, w, x
  __m256 ymm1 = _mm256_permutevar8x32_ps(quat, ymm1Mask);

  // ymm3
  // quater: z, x, y, y
  // quater: y, z, x, z
  __m256 ymm3 = _mm256_permutevar8x32_ps(quat, ymm3Mask);

  // x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
  // y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
  // z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
  // w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;

  __m256 mr0 = _mm256_mul_ps(ymm0, ymm1);
  __m256 mr1 = _mm256_mul_ps(ymm2, ymm3);

  // x = a + b + c - d
  // y = a + b + c - d
  // z = a + b + c - d
  // w = a - b - c - d

  AReg mr0Xor;
  mr0Xor.i = _mm256_set_epi32(0x80000000, 0, 0, 0, 0, 0, 0, 0);
  AReg mr1Xor;
  mr1Xor.i = _mm256_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000,
                              0x80000000, 0, 0, 0);
  __m256 mr0x = _mm256_xor_ps(mr0, mr0Xor.f);
  __m256 mr1x = _mm256_xor_ps(mr1, mr1Xor.f);

  __m256 last = _mm256_add_ps(mr0x, mr1x);
  __m128 lastl = _mm256_extractf128_ps(last, 0);
  __m128 lasth = _mm256_extractf128_ps(last, 1);

  return _mm_add_ps(lastl, lasth);
}

// Multiply array of dq transforms by a single parent
// p.real * o.real, p.real * o.dual + p.dual * o.real
void DualMult(__m256 parent, __m256 *dqs, uint32_t numDqs) {
  __m256i parentQ0Mask{_mm256_set_epi32(0, 2, 1, 0, 3, 3, 3, 3)};
  __m256i parentQ2Mask{_mm256_set_epi32(2, 1, 0, 2, 1, 0, 2, 1)};

  __m256 parentQ0 = _mm256_permutevar8x32_ps(parent, parentQ0Mask);
  __m256 parentQ2 = _mm256_permutevar8x32_ps(parent, parentQ2Mask);

  __m256i parentD0Mask{_mm256_set_epi32(4, 6, 5, 4, 7, 7, 7, 7)};
  __m256i parentD2Mask{_mm256_set_epi32(6, 5, 4, 6, 5, 4, 6, 5)};

  __m256 parentD0 = _mm256_permutevar8x32_ps(parent, parentD0Mask);
  __m256 parentD2 = _mm256_permutevar8x32_ps(parent, parentD2Mask);

  AReg mr0Xor;
  mr0Xor.i = _mm256_set_epi32(0x80000000, 0, 0, 0, 0, 0, 0, 0);
  AReg mr1Xor;
  mr1Xor.i = _mm256_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000,
                              0x80000000, 0, 0, 0);

  __m256i quatQ1Mask{_mm256_set_epi32(0, 3, 3, 3, 3, 2, 1, 0)};
  __m256i quatQ3Mask{_mm256_set_epi32(2, 0, 2, 1, 1, 1, 0, 2)};

  __m256i quatD1Mask{_mm256_set_epi32(4, 7, 7, 7, 7, 6, 5, 4)};
  __m256i quatD3Mask{_mm256_set_epi32(6, 4, 6, 5, 5, 5, 4, 6)};

  while (numDqs) [[likely]] {
    __m256 d = *dqs;
    __m256 quatQ1 = _mm256_permutevar8x32_ps(d, quatQ1Mask);
    __m256 quatQ3 = _mm256_permutevar8x32_ps(d, quatQ3Mask);

    __m256 quatD1 = _mm256_permutevar8x32_ps(d, quatD1Mask);
    __m256 quatD3 = _mm256_permutevar8x32_ps(d, quatD3Mask);

    __m256 qqm0 = _mm256_mul_ps(parentQ0, quatQ1);
    __m256 qqm1 = _mm256_mul_ps(parentQ2, quatQ3);

    __m256 qdm0 = _mm256_mul_ps(parentQ0, quatD1);
    __m256 qdm1 = _mm256_mul_ps(parentQ2, quatD3);

    __m256 dqm0 = _mm256_mul_ps(parentD0, quatQ1);
    __m256 dqm1 = _mm256_mul_ps(parentD2, quatQ3);

    __m256 qqm0x = _mm256_xor_ps(qqm0, mr0Xor.f);
    __m256 qqm1x = _mm256_xor_ps(qqm1, mr1Xor.f);

    __m256 qdm0x = _mm256_xor_ps(qdm0, mr0Xor.f);
    __m256 qdm1x = _mm256_xor_ps(qdm1, mr1Xor.f);

    __m256 dqm0x = _mm256_xor_ps(dqm0, mr0Xor.f);
    __m256 dqm1x = _mm256_xor_ps(dqm1, mr1Xor.f);

    __m256 last0 = _mm256_add_ps(qqm0x, qqm1x);
    __m256 last1 = _mm256_add_ps(dqm0x, dqm1x);
    __m256 last2 = _mm256_add_ps(qdm0x, qdm1x);

    __m256 last3 = _mm256_add_ps(last1, last2);

    __m256 last4 = _mm256_permute2f128_ps(last0, last3, 0b100000);
    __m256 last5 = _mm256_permute2f128_ps(last0, last3, 0b110001);

    *dqs++ = _mm256_add_ps(last4, last5);
    numDqs--;
  }
}
#else
void Decompose8x32b(__m128i input[2], CacheLine out[2]) {
  out[0].s[0].i = _mm_shuffle_epi32(input[0], 0);
  out[0].s[1].i = _mm_shuffle_epi32(input[1], 0);
  out[0].s[2].i = _mm_shuffle_epi32(input[0], 0b0101'0101);
  out[0].s[3].i = _mm_shuffle_epi32(input[1], 0b0101'0101);

  out[1].s[0].i = _mm_shuffle_epi32(input[0], 0b1010'1010);
  out[1].s[1].i = _mm_shuffle_epi32(input[1], 0b1010'1010);
  out[1].s[2].i = _mm_shuffle_epi32(input[0], 0xff);
  out[1].s[3].i = _mm_shuffle_epi32(input[1], 0xff);
}

void Decompose16x16b(__m128i input[2], CacheLine &out) {
  out.s[0].i = _mm_cvtepu16_epi32(input[0]);
  out.s[2].i = _mm_cvtepu16_epi32(input[1]);
  out.s[1].i = _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input[0], input[0]));
  out.s[3].i = _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input[1], input[1]));
}

void Decompose4x16bLow(CacheLine input, CacheLine out[2]) {
  __m128i linel = _mm_cvtepu16_epi32(input.s[0].i);
  __m128i lineh =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[0].i, input.s[0].i));
  __m128i linele = _mm_cvtepu16_epi32(input.s[3].i);
  __m128i linehe =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[3].i, input.s[3].i));

  linel = _mm_or_si128(_mm_slli_epi32(linele, 16), linel);
  lineh = _mm_or_si128(_mm_slli_epi32(linehe, 16), lineh);

  __m128i lines[]{linel, lineh};

  Decompose8x32b(lines, out);
}

void Decompose4x16bMid(CacheLine input, CacheLine out[2]) {
  __m128i linel = _mm_cvtepu16_epi32(input.s[1].i);
  __m128i lineh =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[1].i, input.s[1].i));
  __m128i linele = _mm_cvtepu16_epi32(input.s[3].i);
  __m128i linehe =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[3].i, input.s[3].i));

  linel = _mm_or_si128(_mm_slli_epi32(linele, 16), linel);
  lineh = _mm_or_si128(_mm_slli_epi32(linehe, 16), lineh);

  __m128i lines[]{linel, lineh};

  Decompose8x32b(lines, out);
}

void Decompose4x16bHigh(CacheLine input, CacheLine out[2]) {
  __m128i linel = _mm_cvtepu16_epi32(input.s[2].i);
  __m128i lineh =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[2].i, input.s[2].i));
  __m128i linele = _mm_cvtepu16_epi32(input.s[3].i);
  __m128i linehe =
      _mm_cvtepu16_epi32(_mm_unpackhi_epi64(input.s[3].i, input.s[3].i));

  linel = _mm_or_si128(_mm_slli_epi32(linele, 16), linel);
  lineh = _mm_or_si128(_mm_slli_epi32(linehe, 16), lineh);

  __m128i lines[]{linel, lineh};

  Decompose8x32b(lines, out);
}

void ConvertCacheline4x4(CacheLine line, CacheLine scaleF) {
  for (int i = 0; i < 4; i++) {
    line.s[i].f = _mm_mul_ps(_mm_cvtepi32_ps(line.s[i].i), scaleF.s[i].f);
  }
}

void ConvertCacheline4x4(CacheLine line, CacheLine shiftI, CacheLine scaleF) {
  for (int i = 0; i < 4; i++) {
    line.s[i].i = _mm_srli_epi32(_mm_mul_epu32(line.s[i].i, shiftI.s[i].i), 16);
    line.s[i].f = _mm_mul_ps(_mm_cvtepi32_ps(line.s[i].i), scaleF.s[i].f);
  }
}

// Compute W component (4th) for 4 packed quaternions
// sqrt(1 - q.xyz . q.xyz)
void ComputeQuatReal(CacheLine &line) {
  __m128 dp0 = _mm_dp_ps(line.s[0].f, line.s[0].f, 0b111'0001);
  __m128 dp1 = _mm_dp_ps(line.s[1].f, line.s[1].f, 0b111'0010);
  __m128 dp2 = _mm_dp_ps(line.s[2].f, line.s[2].f, 0b111'0100);
  __m128 dp3 = _mm_dp_ps(line.s[3].f, line.s[3].f, 0b111'1000);

  __m128 line0 = _mm_or_ps(dp0, dp1);
  __m128 line1 = _mm_or_ps(dp2, dp3);
  __m128 lineL = _mm_or_ps(line0, line1);

  __m128 linelSub = _mm_sub_ps(_mm_set1_ps(1), lineL);
  __m128 result = _mm_sqrt_ps(linelSub);

  line.s[0].f = _mm_insert_ps(line.s[0].f, result, 0b0011'0000);
  line.s[1].f = _mm_insert_ps(line.s[1].f, result, 0b0111'0000);
  line.s[2].f = _mm_insert_ps(line.s[2].f, result, 0b1011'0000);
  line.s[3].f = _mm_insert_ps(line.s[3].f, result, 0b1111'0000);
}

__m128 QuatMult(__m128 parent, __m128 quat) {
  __m128 ymm00 = _mm_shuffle_ps(parent, parent, _MM_SHUFFLE(3, 3, 3, 3));
  __m128 ymm01 = _mm_shuffle_ps(parent, parent, _MM_SHUFFLE(0, 2, 1, 0));
  __m128 ymm20 = _mm_shuffle_ps(parent, parent, _MM_SHUFFLE(1, 0, 2, 1));
  __m128 ymm21 = _mm_shuffle_ps(parent, parent, _MM_SHUFFLE(2, 1, 0, 2));

  __m128 ymm10 = _mm_shuffle_ps(quat, quat, _MM_SHUFFLE(3, 2, 1, 0));
  __m128 ymm11 = _mm_shuffle_ps(quat, quat, _MM_SHUFFLE(0, 3, 3, 3));
  __m128 ymm30 = _mm_shuffle_ps(quat, quat, _MM_SHUFFLE(1, 1, 0, 2));
  __m128 ymm31 = _mm_shuffle_ps(quat, quat, _MM_SHUFFLE(2, 0, 2, 1));

  __m128 mr00 = _mm_mul_ps(ymm00, ymm10);
  __m128 mr01 = _mm_mul_ps(ymm01, ymm11);
  __m128 mr10 = _mm_mul_ps(ymm20, ymm30);
  __m128 mr11 = _mm_mul_ps(ymm21, ymm31);

  SReg mr00Xor;
  mr00Xor.i = _mm_set_epi32(0, 0, 0, 0);
  SReg mr01Xor;
  mr01Xor.i = _mm_set_epi32(0x80000000, 0, 0, 0);
  SReg mr10Xor;
  mr10Xor.i = _mm_set_epi32(0x80000000, 0, 0, 0);
  SReg mr11Xor;
  mr11Xor.i = _mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);

  __m128 mr00x = _mm_xor_ps(mr00, mr00Xor.f);
  __m128 mr01x = _mm_xor_ps(mr01, mr01Xor.f);
  __m128 mr10x = _mm_xor_ps(mr10, mr10Xor.f);
  __m128 mr11x = _mm_xor_ps(mr11, mr11Xor.f);

  __m128 lastl = _mm_add_ps(mr00x, mr01x);
  __m128 lasth = _mm_add_ps(mr10x, mr11x);
  return _mm_add_ps(lastl, lasth);
}
#endif

struct BReg32 {
  uint32_t values[4];
};

struct BReg16 {
  uint16_t values[8];
};

union TsReg {
  BReg32 u32;
  BReg16 u16;
  SReg xmm;

  TsReg() = default;
  TsReg(uint32_t s) : u32{s, s, s, s} {}
  TsReg(uint16_t s, uint16_t t) : u16{s, t, s, t, s, t, s, t} {}
};

union AsReg {
  TsReg d[2];
  AReg ymm;
};

// Multiply array of dq transforms by a single parent
// p.real * o.real, p.real * o.dual + p.dual * o.real
void DualMult(__m128 parent[2], __m128 *dqs, uint32_t numDqs) {
  __m128 parentQ00 =
      _mm_shuffle_ps(parent[0], parent[0], _MM_SHUFFLE(3, 3, 3, 3));
  __m128 parentQ01 =
      _mm_shuffle_ps(parent[0], parent[0], _MM_SHUFFLE(0, 2, 1, 0));
  __m128 parentQ20 =
      _mm_shuffle_ps(parent[0], parent[0], _MM_SHUFFLE(1, 0, 2, 1));
  __m128 parentQ21 =
      _mm_shuffle_ps(parent[0], parent[0], _MM_SHUFFLE(2, 1, 0, 2));

  __m128 parentD00 =
      _mm_shuffle_ps(parent[1], parent[1], _MM_SHUFFLE(3, 3, 3, 3));
  __m128 parentD01 =
      _mm_shuffle_ps(parent[1], parent[1], _MM_SHUFFLE(0, 2, 1, 0));
  __m128 parentD20 =
      _mm_shuffle_ps(parent[1], parent[1], _MM_SHUFFLE(1, 0, 2, 1));
  __m128 parentD21 =
      _mm_shuffle_ps(parent[1], parent[1], _MM_SHUFFLE(2, 1, 0, 2));

  SReg mr00Xor;
  mr00Xor.i = _mm_set_epi32(0, 0, 0, 0);
  SReg mr01Xor;
  mr01Xor.i = _mm_set_epi32(0x80000000, 0, 0, 0);
  SReg mr10Xor;
  mr10Xor.i = _mm_set_epi32(0x80000000, 0, 0, 0);
  SReg mr11Xor;
  mr11Xor.i = _mm_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);

  while (numDqs) [[likely]] {
    __m128 quatQ10 = _mm_shuffle_ps(dqs[0], dqs[0], _MM_SHUFFLE(3, 2, 1, 0));
    __m128 quatQ11 = _mm_shuffle_ps(dqs[0], dqs[0], _MM_SHUFFLE(0, 3, 3, 3));
    __m128 quatQ30 = _mm_shuffle_ps(dqs[0], dqs[0], _MM_SHUFFLE(1, 1, 0, 2));
    __m128 quatQ31 = _mm_shuffle_ps(dqs[0], dqs[0], _MM_SHUFFLE(2, 0, 2, 1));

    __m128 quatD10 = _mm_shuffle_ps(dqs[1], dqs[1], _MM_SHUFFLE(3, 2, 1, 0));
    __m128 quatD11 = _mm_shuffle_ps(dqs[1], dqs[1], _MM_SHUFFLE(0, 3, 3, 3));
    __m128 quatD30 = _mm_shuffle_ps(dqs[1], dqs[1], _MM_SHUFFLE(1, 1, 0, 2));
    __m128 quatD31 = _mm_shuffle_ps(dqs[1], dqs[1], _MM_SHUFFLE(2, 0, 2, 1));

    {
      __m128 mr00 = _mm_mul_ps(parentQ00, quatQ10);
      __m128 mr01 = _mm_mul_ps(parentQ01, quatQ11);
      __m128 mr10 = _mm_mul_ps(parentQ20, quatQ30);
      __m128 mr11 = _mm_mul_ps(parentQ21, quatQ31);

      __m128 mr00x = _mm_xor_ps(mr00, mr00Xor.f);
      __m128 mr01x = _mm_xor_ps(mr01, mr01Xor.f);
      __m128 mr10x = _mm_xor_ps(mr10, mr10Xor.f);
      __m128 mr11x = _mm_xor_ps(mr11, mr11Xor.f);

      __m128 lastl = _mm_add_ps(mr00x, mr01x);
      __m128 lasth = _mm_add_ps(mr10x, mr11x);
      dqs[0] = _mm_add_ps(lastl, lasth);
    }

    {
      __m128 mr00 = _mm_mul_ps(parentQ00, quatD10);
      __m128 mr01 = _mm_mul_ps(parentQ01, quatD11);
      __m128 mr10 = _mm_mul_ps(parentQ20, quatD30);
      __m128 mr11 = _mm_mul_ps(parentQ21, quatD31);

      __m128 mr00x = _mm_xor_ps(mr00, mr00Xor.f);
      __m128 mr01x = _mm_xor_ps(mr01, mr01Xor.f);
      __m128 mr10x = _mm_xor_ps(mr10, mr10Xor.f);
      __m128 mr11x = _mm_xor_ps(mr11, mr11Xor.f);

      __m128 lastl = _mm_add_ps(mr00x, mr01x);
      __m128 lasth = _mm_add_ps(mr10x, mr11x);
      dqs[1] = _mm_add_ps(lastl, lasth);
    }

    {
      __m128 mr00 = _mm_mul_ps(parentD00, quatQ10);
      __m128 mr01 = _mm_mul_ps(parentD01, quatQ11);
      __m128 mr10 = _mm_mul_ps(parentD20, quatQ30);
      __m128 mr11 = _mm_mul_ps(parentD21, quatQ31);

      __m128 mr00x = _mm_xor_ps(mr00, mr00Xor.f);
      __m128 mr01x = _mm_xor_ps(mr01, mr01Xor.f);
      __m128 mr10x = _mm_xor_ps(mr10, mr10Xor.f);
      __m128 mr11x = _mm_xor_ps(mr11, mr11Xor.f);

      __m128 lastl = _mm_add_ps(mr00x, mr01x);
      __m128 lasth = _mm_add_ps(mr10x, mr11x);
      dqs[1] = _mm_add_ps(_mm_add_ps(lastl, lasth), dqs[1]);
    }

    dqs += 2;
    numDqs--;
  }
}

#include <stdexcept>

void CompareLines(CacheLine line, CacheLine expected) {
  for (int i = 0; i < 4; i++) {
    SReg result;
    TsReg dbg;
    dbg.xmm = line.s[i];
    result.i = _mm_cmpeq_epi64(line.s[i].i, expected.s[i].i);
    if (_mm_movemask_ps(result.f) != 0xf) {
      throw std::runtime_error("Comparison failed");
    }
  }
}

int TestDecompose4x32b() {
  TsReg rg;
  rg.u32 = {1, 2, 3, 4};
  CacheLine line;
  Decompose4x32b(rg.xmm.i, line);
  TsReg e0(1);
  TsReg e1(2);
  TsReg e2(3);
  TsReg e3(4);
  CacheLine expectedLine{e0.xmm, e1.xmm, e2.xmm, e3.xmm};
  CompareLines(line, expectedLine);
  return 0;
}

int TestDecompose16x16b() {
  TsReg rg0;
  rg0.u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  TsReg rg1;
  rg1.u16 = {9, 10, 11, 12, 13, 14, 15, 16};

  __m128i rg[]{rg0.xmm.i, rg1.xmm.i};
  CacheLine line;
  Decompose16x16b(rg, line);
  TsReg e0;
  e0.u32 = {1, 2, 3, 4};
  TsReg e1;
  e1.u32 = {5, 6, 7, 8};
  TsReg e2;
  e2.u32 = {9, 10, 11, 12};
  TsReg e3;
  e3.u32 = {13, 14, 15, 16};
  CacheLine expectedLine{e0.xmm, e1.xmm, e2.xmm, e3.xmm};
  CompareLines(line, expectedLine);
  return 0;
}

#include <cstring>

int TestDecompose2AReg8x4() {
  TsReg rg0;
  rg0.u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  TsReg rg1;
  rg1.u16 = {9, 10, 11, 12, 13, 14, 15, 16};
  TsReg rg2;
  rg2.u16 = {17, 18, 19, 20, 21, 22, 23, 24};
  TsReg rg3;
  rg3.u16 = {25, 26, 27, 28, 29, 30, 31, 32};

  CacheLine rg;
  SReg rgs[4]{rg0.xmm, rg1.xmm, rg2.xmm, rg3.xmm};
  memcpy(rg.s, rgs, sizeof(CacheLine));
  CacheLine line[2];

  Decompose4x16bLow(rg, line);

  TsReg e00(1, 25);
  TsReg e01(2, 26);
  TsReg e02(3, 27);
  TsReg e03(4, 28);
  TsReg e10(5, 29);
  TsReg e11(6, 30);
  TsReg e12(7, 31);
  TsReg e13(8, 32);
  {
    CacheLine expectedLine{e00.xmm, e10.xmm, e01.xmm, e11.xmm};
    CompareLines(line[0], expectedLine);
  }

  {
    CacheLine expectedLine{e02.xmm, e12.xmm, e03.xmm, e13.xmm};
    CompareLines(line[1], expectedLine);
  }

  Decompose4x16bMid(rg, line);

  TsReg e20(9, 25);
  TsReg e21(10, 26);
  TsReg e22(11, 27);
  TsReg e23(12, 28);
  TsReg e30(13, 29);
  TsReg e31(14, 30);
  TsReg e32(15, 31);
  TsReg e33(16, 32);
  {
    CacheLine expectedLine{e20.xmm, e30.xmm, e21.xmm, e31.xmm};
    CompareLines(line[0], expectedLine);
  }

  {
    CacheLine expectedLine{e22.xmm, e32.xmm, e23.xmm, e33.xmm};
    CompareLines(line[1], expectedLine);
  }

  Decompose4x16bHigh(rg, line);

  TsReg e40(17, 25);
  TsReg e41(18, 26);
  TsReg e42(19, 27);
  TsReg e43(20, 28);
  TsReg e50(21, 29);
  TsReg e51(22, 30);
  TsReg e52(23, 31);
  TsReg e53(24, 32);
  {
    CacheLine expectedLine{e40.xmm, e50.xmm, e41.xmm, e51.xmm};
    CompareLines(line[0], expectedLine);
  }

  {
    CacheLine expectedLine{e42.xmm, e52.xmm, e43.xmm, e53.xmm};
    CompareLines(line[1], expectedLine);
  }

  return 0;
}

#include <chrono>

struct Histogram {
  uint32_t le10 = 0;
  uint32_t le15 = 0;
  uint32_t le20 = 0;
  uint32_t le25 = 0;
  uint32_t le30 = 0;
  uint32_t le50 = 0;
  uint32_t le70 = 0;
  uint32_t le100 = 0;
  uint32_t le200 = 0;
  uint32_t le300 = 0;
  uint32_t le500 = 0;
  uint32_t le1000 = 0;
  uint32_t le1inf = 0;
};

union HistogramU {
  Histogram h;
  uint32_t b[13];
};

void Observe(HistogramU &h, size_t value) {
  static const uint32_t buckets[]{10, 15,  20,  25,  30,  50,
                                  70, 100, 200, 300, 500, 1000};

  for (uint32_t i = 0; i < 12; i++) {
    if (buckets[i] > value) {
      h.b[i]++;
      return;
    }
  }

  h.h.le1inf++;
}

void Print(HistogramU &h, const char *name) {
  static const uint32_t buckets[]{10, 15,  20,  25,  30,  50,
                                  70, 100, 200, 300, 500, 1000};

  for (uint32_t i = 0; i < 12; i++) {
    printf("[%s] Bucket <%u ns: %f %%\n", name, buckets[i], h.b[i] / 200'000.f);
  }

  printf("[%s] Bucket <inf: %f %%\n", name, h.h.le1inf / 200'000.f);
}

void Bench4x32b() {
  TsReg rg;
  rg.u32 = {1, 2, 3, 4};
  CacheLine line;
  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Decompose4x32b(rg.xmm.i, line);
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "4x32b");
  printf("[4x32b] Average duration: %zu ns\n", micros / 20'000'000);
}

void Bench16x16b() {
  TsReg rg0;
  rg0.u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  TsReg rg1;
  rg1.u16 = {9, 10, 11, 12, 13, 14, 15, 16};

  __m128i rg[]{rg0.xmm.i, rg1.xmm.i};
  CacheLine line;
  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Decompose16x16b(rg, line);
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "16x16b");
  printf("[16x16b] Average duration: %zu ns\n", micros / 20'000'000);
}

void Bench2AReg8x4() {
  TsReg rg0;
  rg0.u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  TsReg rg1;
  rg1.u16 = {9, 10, 11, 12, 13, 14, 15, 16};
  TsReg rg2;
  rg2.u16 = {17, 18, 19, 20, 21, 22, 23, 24};
  TsReg rg3;
  rg3.u16 = {25, 26, 27, 28, 29, 30, 31, 32};

  CacheLine rg;
  SReg rgs[4]{rg0.xmm, rg1.xmm, rg2.xmm, rg3.xmm};
  memcpy(rg.s, rgs, sizeof(CacheLine));
  CacheLine line[2];

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Decompose4x16bLow(rg, line);
    Decompose4x16bMid(rg, line);
    Decompose4x16bHigh(rg, line);
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "2AReg8x4");
  printf("[2AReg8x4] Average duration: %zu ns\n", micros / 20'000'000);
}

void BenchQuatMult() {
  AsReg rg;
  rg.d[0].u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  rg.d[0].u16 = {9, 10, 11, 12, 13, 14, 15, 16};

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
#ifdef __AVX2__
    QuatMult(rg.ymm.f, rg.ymm.f);
#else
    QuatMult(rg.d->xmm.f, rg.d->xmm.f);
#endif
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "QuatMult");
  printf("[QuatMult] Average duration: %zu ns\n", micros / 20'000'000);
}

void BenchDualMult() {
  AsReg rg;
  rg.d[0].u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  rg.d[0].u16 = {9, 10, 11, 12, 13, 14, 15, 16};

  AsReg rg0;
  rg0.d[0].u16 = {1, 2, 3, 4, 5, 6, 7, 8};
  rg0.d[0].u16 = {9, 10, 11, 12, 13, 14, 15, 16};

  AsReg rgn[255];

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    for (auto &r : rgn) {
      r = rg0;
    }
    auto startTime = std::chrono::high_resolution_clock::now();
#ifdef __AVX2__
    DualMult(rg.ymm.f, &rgn->ymm.f, 255);
#else
    DualMult(&rg.d->xmm.f, &rgn->d->xmm.f, 255);
#endif
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "DualMult");
  printf("[DualMult] Average duration: %zu ns\n", micros / 20'000'000);
}

#include "glm/gtx/dual_quaternion.hpp"

static int myrand() {
  static int holdrand = 1;
  return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

static float myfrand() // returns values from -1 to 1 inclusive
{
  return float(double(myrand()) / double(0x7ffff)) * 2.0f - 1.0f;
}

static float mytrand() { return float(double(myrand()) / double(0xffff)); }

void SmokeTestQuatMult() {
  union QReg {
    glm::quat q;
    __m128 r;
    __m256 f;
  };

  for (int j = 0; j < 100; ++j) {
    QReg dq0{glm::quat(glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                                           myfrand() * glm::pi<float>(),
                                           myfrand() * glm::pi<float>())))};
    QReg dq1{glm::quat(glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                                           myfrand() * glm::pi<float>(),
                                           myfrand() * glm::pi<float>())))};
#ifdef __AVX2__
    QReg ret{.r = QuatMult(dq0.f, dq1.f)};
#else
    QReg ret{.r = QuatMult(dq0.r, dq1.r)};
#endif
    glm::quat mult = dq0.q * dq1.q;
    float const Epsilon = 0.0001f;
    assert(glm::all(glm::epsilonEqual(mult, ret.q, Epsilon)));
  }
}

void SmokeTestDualQuatMult() {
  union QReg {
    glm::dualquat dq;
    __m128 r;
    __m128 r2[2];
    __m256 f;
  };

  for (int j = 0; j < 100; ++j) {
    QReg dq0{
        glm::dualquat(glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                                          myfrand() * glm::pi<float>(),
                                          myfrand() * glm::pi<float>())),
                      glm::quat(glm::vec3(mytrand(), mytrand(), mytrand())))};

    QReg dq1[100];
    QReg dq2[100];

    for (int k = 0; k < 100; ++k) {
      dq1[k].dq =
          glm::dualquat(glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                                            myfrand() * glm::pi<float>(),
                                            myfrand() * glm::pi<float>())),
                        glm::quat(glm::vec3(mytrand(), mytrand(), mytrand())));
      dq2[k].dq = dq1[k].dq;
    }

#ifdef __AVX2__
    DualMult(dq0.f, &dq1->f, 100);
#else
    DualMult(dq0.r2, &dq1->r, 100);
#endif

    for (int k = 0; k < 100; ++k) {
      glm::dualquat mult = dq0.dq * dq2[k].dq;

      float const Epsilon = 0.0001f;
      assert(glm::all(glm::epsilonEqual(mult.real, dq1[k].dq.real, Epsilon)));
      assert(glm::all(glm::epsilonEqual(mult.dual, dq1[k].dq.dual, Epsilon)));
    }
  }
}

void SmokeTestComputeQuatReal() {
  for (int j = 0; j < 100; ++j) {
    union CReg {
      glm::quat qt[4];
      CacheLine line;
    };

    CReg reg{
        glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>())),
        glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>())),
        glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>())),
        glm::quat(glm::vec3(myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>(),
                            myfrand() * glm::pi<float>())),
    };

    for (auto &q : reg.qt) {
      if (q.z < 0) {
        q = -q;
      }
    }

    CacheLine line = reg.line;
    ComputeQuatReal(line);
    CReg lineReg{.line = line};

    float const Epsilon = 0.0001f;

    for (int i = 0; i < 4; i++) {
      assert(glm::all(glm::epsilonEqual(lineReg.qt[i], reg.qt[i], Epsilon)));
    }
  }
}

void TestComputeQuatRealZero() {
  union CReg {
    glm::quat qt[4];
    CacheLine line;
  };

  CReg reg{
      glm::quat(1, 0, 0, 0),
      glm::quat(0, 1, 0, 0),
      glm::quat(0, 0, 1, 0),
      glm::quat(0, 0, 0, 1),
  };

  CacheLine line = reg.line;
  ComputeQuatReal(line);
  CReg lineReg{.line = line};

  float const Epsilon = 0.0001f;

  for (int i = 0; i < 4; i++) {
    assert(glm::all(glm::epsilonEqual(lineReg.qt[i], reg.qt[i], Epsilon)));
  }
}

void BenchComputeQuatReal() {
  union CReg {
    float f[16];
    CacheLine line;
  };

  CReg reg{
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ComputeQuatReal(reg.line);
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "ComputeQuatReal");
  printf("[ComputeQuatReal] Average duration: %zu ns\n", micros / 20'000'000);
}

int main() {
  TestDecompose4x32b();
  TestDecompose16x16b();
  TestDecompose2AReg8x4();
  SmokeTestQuatMult();
  SmokeTestDualQuatMult();
  SmokeTestComputeQuatReal();
  TestComputeQuatRealZero();

  BenchQuatMult();        // sse 17, avx 17
  BenchDualMult();        // sse 691 avx 683
  BenchComputeQuatReal(); // sse 17

  Bench4x32b();    // sse 17
  Bench16x16b();   // sse 17, avx 17
  Bench2AReg8x4(); // sse 18, avx 17*
  return 0;
}
