#include <cstdint>
#include <immintrin.h>

#define USE_SHUFFLE

union SReg {
  __m128 f;
  __m128i i;
};

union AReg {
  SReg r[2];
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
  float fl[8];
  float f[16];
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

void CacheLineToPoseTable(CacheLine line, float *poseTable,
                          const uint32_t *ptIndices) {
  for (float f : line.f) {
    poseTable[*ptIndices++] = f;
  }
}
void LowCacheLineToPoseTable(CacheLine line, float *poseTable,
                             const uint32_t *ptIndices) {
  for (float f : line.fl) {
    poseTable[*ptIndices++] = f;
  }
}

#ifdef __AVX2__
void Decompose8x32b(AReg input, CacheLine out[2]) {
  out[0].a[0].i = _mm256_shuffle_epi32(input.i, 0);
  out[0].a[1].i = _mm256_shuffle_epi32(input.i, 0b0101'0101);
  out[1].a[0].i = _mm256_shuffle_epi32(input.i, 0b1010'1010);
  out[1].a[1].i = _mm256_shuffle_epi32(input.i, 0xff);
}

void Decompose16x16b(const __m128i input[2], CacheLine &out) {
  out.a[0].i = _mm256_cvtepu16_epi32(input[0]);
  out.a[1].i = _mm256_cvtepu16_epi32(input[1]);
}

void Decompose2x16bLow(const __m128i input[2], CacheLine out[2]) {
  __m256i line0 = _mm256_cvtepu16_epi32(input[0]);
  __m256i line1 = _mm256_cvtepu16_epi32(input[1]);
  line1 = _mm256_slli_epi32(line1, 16);

  AReg lineo;
  lineo.i = _mm256_or_si256(line0, line1);

  Decompose8x32b(lineo, out);
}

void Decompose2x16bHigh(const __m128i input[2], CacheLine out[2]) {
  __m256i line0 = _mm256_cvtepu16_epi32(input[1]);
  __m256i line1 = _mm256_cvtepu16_epi32(input[0]);
  line1 = _mm256_slli_epi32(line1, 16);

  AReg lineo;
  lineo.i = _mm256_or_si256(line0, line1);

  Decompose8x32b(lineo, out);
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

void LerpCacheLines(CacheLine lines[2], float delta) {
  __m256 part00 = _mm256_sub_ps(lines[1].a[0].f, lines[0].a[0].f);
  __m256 part01 = _mm256_sub_ps(lines[1].a[1].f, lines[0].a[1].f);
  __m256 deltaV = _mm256_set1_ps(delta);

  lines[0].a[0].f =
      _mm256_add_ps(lines[0].a[0].f, _mm256_mul_ps(part00, deltaV));
  lines[0].a[1].f =
      _mm256_add_ps(lines[0].a[1].f, _mm256_mul_ps(part01, deltaV));
}

void LerpCacheLine(CacheLine line, float delta) {
  __m256 part00 = _mm256_sub_ps(line.a[1].f, line.a[0].f);
  __m256 deltaV = _mm256_set1_ps(delta);

  line.a[0].f = _mm256_add_ps(line.a[0].f, _mm256_mul_ps(part00, deltaV));
}

void LerpCacheLines2(CacheLine lines[4], float delta) {
  __m256 part00 = _mm256_sub_ps(lines[2].a[0].f, lines[0].a[0].f);
  __m256 part01 = _mm256_sub_ps(lines[2].a[1].f, lines[0].a[1].f);
  __m256 part10 = _mm256_sub_ps(lines[3].a[0].f, lines[1].a[0].f);
  __m256 part11 = _mm256_sub_ps(lines[3].a[1].f, lines[1].a[1].f);
  __m256 deltaV = _mm256_set1_ps(delta);

  lines[0].a[0].f =
      _mm256_add_ps(lines[0].a[0].f, _mm256_mul_ps(part00, deltaV));
  lines[0].a[1].f =
      _mm256_add_ps(lines[0].a[1].f, _mm256_mul_ps(part01, deltaV));
  lines[1].a[0].f =
      _mm256_add_ps(lines[1].a[0].f, _mm256_mul_ps(part10, deltaV));
  lines[1].a[1].f =
      _mm256_add_ps(lines[1].a[1].f, _mm256_mul_ps(part11, deltaV));
}

// Compute W component for 4 packed quaternions
// sqrt(1 - q.xyz . q.xyz)
void ComputeQuatReal(CacheLine &line) {
#ifdef QUAT_LAYOUT_REAL_FRONT
  const int DP_MASK0 = 0b1110'0101;
  const int DP_MASK1 = 0b1110'1010;
  static const __m256i PM_MASK0 = _mm256_set_epi32(0, 0, 0, 6, 0, 0, 0, 0);
  static const __m256i PM_MASK1 = _mm256_set_epi32(0, 0, 0, 7, 0, 0, 0, 1);
  const int BD_MASK = 0x11;
#else
  const int DP_MASK0 = 0b111'0101;
  const int DP_MASK1 = 0b111'1010;
  static const __m256i PM_MASK0 = _mm256_set_epi32(6, 0, 0, 0, 0, 0, 0, 0);
  static const __m256i PM_MASK1 = _mm256_set_epi32(7, 0, 0, 0, 1, 0, 0, 0);
  const int BD_MASK = 0x88;
#endif
  __m256 ldp = _mm256_dp_ps(line.a[0].f, line.a[0].f, DP_MASK0);
  __m256 hdp = _mm256_dp_ps(line.a[1].f, line.a[1].f, DP_MASK1);
  __m256 mdp = _mm256_or_ps(ldp, hdp);

  __m256 linelSub = _mm256_sub_ps(_mm256_set1_ps(1), mdp);
  __m256 result = _mm256_sqrt_ps(linelSub);
  __m256 sdpl = _mm256_permutevar8x32_ps(result, PM_MASK0);
  __m256 sdph = _mm256_permutevar8x32_ps(result, PM_MASK1);
  line.a[0].f = _mm256_blend_ps(line.a[0].f, sdpl, BD_MASK);
  line.a[1].f = _mm256_blend_ps(line.a[1].f, sdph, BD_MASK);
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

// Compute W component for 4 packed quaternions
// sqrt(1 - q.xyz . q.xyz)
void ComputeQuatReal(CacheLine &line) {
#ifdef QUAT_LAYOUT_REAL_FRONT
  const int DP_MASK0 = 0b1110'0001;
  const int DP_MASK1 = 0b1110'0010;
  const int DP_MASK2 = 0b1110'0100;
  const int DP_MASK3 = 0b1110'1000;
  const int IT_MASK0 = 0b0000'0000;
  const int IT_MASK1 = 0b0100'0000;
  const int IT_MASK2 = 0b1000'0000;
  const int IT_MASK3 = 0b1100'0000;
#else
  const int DP_MASK0 = 0b111'0001;
  const int DP_MASK1 = 0b111'0010;
  const int DP_MASK2 = 0b111'0100;
  const int DP_MASK3 = 0b111'1000;
  const int IT_MASK0 = 0b0011'0000;
  const int IT_MASK1 = 0b0111'0000;
  const int IT_MASK2 = 0b1011'0000;
  const int IT_MASK3 = 0b1111'0000;
#endif

  __m128 dp0 = _mm_dp_ps(line.s[0].f, line.s[0].f, DP_MASK0);
  __m128 dp1 = _mm_dp_ps(line.s[1].f, line.s[1].f, DP_MASK1);
  __m128 dp2 = _mm_dp_ps(line.s[2].f, line.s[2].f, DP_MASK2);
  __m128 dp3 = _mm_dp_ps(line.s[3].f, line.s[3].f, DP_MASK3);

  __m128 line0 = _mm_or_ps(dp0, dp1);
  __m128 line1 = _mm_or_ps(dp2, dp3);
  __m128 lineL = _mm_or_ps(line0, line1);

  __m128 linelSub = _mm_sub_ps(_mm_set1_ps(1), lineL);
  __m128 result = _mm_sqrt_ps(linelSub);

  line.s[0].f = _mm_insert_ps(line.s[0].f, result, IT_MASK0);
  line.s[1].f = _mm_insert_ps(line.s[1].f, result, IT_MASK1);
  line.s[2].f = _mm_insert_ps(line.s[2].f, result, IT_MASK2);
  line.s[3].f = _mm_insert_ps(line.s[3].f, result, IT_MASK3);
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

struct BReg8 {
  uint8_t values[16];
};

struct BReg32x2 {
  uint32_t values[8];
};

struct BReg64 {
  uint64_t values[2];
};

struct BReg64x2 {
  uint64_t values[4];
};

struct BReg16x2 {
  uint16_t values[16];
};

struct BRegFloat {
  float values[4];
};

struct BRegFloatx2 {
  float values[8];
};

union TsReg {
  BReg64 u64;
  BRegFloat f;
  BReg32 u32;
  BReg16 u16;
  BReg8 u8;
  SReg xmm;

  TsReg() = default;
  TsReg(uint32_t s) : u32{s, s, s, s} {}
  TsReg(uint16_t s, uint16_t t) : u16{s, t, s, t, s, t, s, t} {}
};

union AsReg {
  TsReg d[2];
  BRegFloatx2 f;
  BReg32x2 u32;
  BReg16x2 u16;
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
    [[maybe_unused]] volatile CacheLine linev = line;
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
    [[maybe_unused]] volatile CacheLine linev = line;
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
    [[maybe_unused]] volatile CacheLine linev = rg;
  }

  Print(h, "2AReg8x4");
  printf("[2AReg8x4] Average duration: %zu ns\n", micros / 20'000'000);
}

void BenchQuatMult() {
  AsReg rg;
  rg.u16 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] volatile auto item =
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
  rg.u16 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

  AsReg rg0;
  rg0.u16 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

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

#define GLM_FORCE_QUAT_DATA_XYZW
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
    [[maybe_unused]] volatile CacheLine linev = reg.line;
  }

  Print(h, "ComputeQuatReal");
  printf("[ComputeQuatReal] Average duration: %zu ns\n", micros / 20'000'000);
}

union BlockDelta8 {
  struct {
    uint16_t blockId;
    uint16_t alpha;
  } r[8];
  __m256 ymm;
};

BlockDelta8 BlockDelta8x8(uint32_t frameBlock[8], uint8_t blockOffsets[8],
                          uint32_t localFrame) {
  BlockDelta8 retval{};

  for (int i = 0; i < 8; i++) {
    uint32_t frames = frameBlock[i];
    retval.r[i].blockId = blockOffsets[i];

    for (uint32_t j = 0; j < localFrame; j++) {
      retval.r[i].blockId += frames & (1 << j);
    }

    for (int j = localFrame - 1; j > 0; j--) {
      if (frames & (1 << j)) {
        retval.r[i].alpha = j;
        break;
      }
    }
  }

  return retval;
}

#ifdef __AVX__

__m256i Log2(__m256i input) {
  input = _mm256_or_si256(input, _mm256_srli_epi32(input, 1));
  input = _mm256_or_si256(input, _mm256_srli_epi32(input, 2));
  input = _mm256_or_si256(input, _mm256_srli_epi32(input, 4));
  input = _mm256_or_si256(input, _mm256_srli_epi32(input, 8));
  input = _mm256_or_si256(input, _mm256_srli_epi32(input, 16));
  input = _mm256_mullo_epi32(input, _mm256_set1_epi32(0x07C4ACDDU));
  input = _mm256_srli_epi32(input, 27);

  static const int multiplyDeBruijnBitPosition[32] = {
      0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
      8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

  return _mm256_i32gather_epi32(multiplyDeBruijnBitPosition, input, 4);
}

void TestLog2() {
  AsReg reg;
  reg.u32 = {1029870208, 1114013593, 3708233265, 776968483,
             1653167050, 4059207886, 3779785463, 3330665569};

  AsReg ret;
  ret.ymm.i = Log2(reg.ymm.i);

  for (int i = 0; i < 8; i++) {
    uint32_t bit = 0;
    uint32_t item = reg.u32.values[i];

    while (item >>= 1) {
      bit++;
    }

    assert(bit == ret.u32.values[i]);
  }
}

__m256i NumBits(__m256i input) {
  // v = v - ((v >> 1) & 0x55555555);
  input =
      _mm256_sub_epi32(input, _mm256_and_si256(_mm256_srli_epi32(input, 1),
                                               _mm256_set1_epi32(0x55555555)));
  // u = (v >> 2) & 0x33333333;
  __m256i u = _mm256_and_si256(_mm256_srli_epi32(input, 2),
                               _mm256_set1_epi32(0x33333333));
  // v = (v & 0x33333333) + u;
  input = _mm256_add_epi32(
      _mm256_and_si256(input, _mm256_set1_epi32(0x33333333)), u);
  // u = v + (v >> 4) & 0xF0F0F0F;
  u = _mm256_and_si256(_mm256_add_epi32(input, _mm256_srli_epi32(input, 4)),
                       _mm256_set1_epi32(0xF0F0F0F));

  // v = (u * 0x1010101) >> 24;
  return _mm256_srli_epi32(_mm256_mullo_epi32(u, _mm256_set1_epi32(0x1010101)),
                           24);
}

void TestNumBits() {
  AsReg reg;
  reg.u32 = {1029870208, 1114013593, 3708233265, 776968483,
             1653167050, 4059207886, 3779785463, 3330665569};

  AsReg ret;
  ret.ymm.i = NumBits(reg.ymm.i);

  for (int i = 0; i < 8; i++) {
    uint32_t bits = 0;
    uint32_t item = reg.u32.values[i];

    do {
      bits += item & 1;
    } while (item >>= 1);

    assert(bits == ret.u32.values[i]);
  }
}

__m256i NumLeadingZeroes(__m256i input) {
  input =
      _mm256_and_si256(input, _mm256_sub_epi32(_mm256_set1_epi32(0), input));
  input = _mm256_srli_epi32(
      _mm256_mullo_epi32(input, _mm256_set1_epi32(0x077CB531U)), 27);

  static const int multiplyDeBruijnBitPosition[32] = {
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
  return _mm256_i32gather_epi32(multiplyDeBruijnBitPosition, input, 4);
}

void TestNumLeadingZeroes() {
  AsReg reg;
  reg.u32 = {0xff, 0xfc, 0x80, 0x7000, 0xf1500000, 0x5800a000, 0xff000100, 0};

  AsReg ret;
  ret.ymm.i = NumLeadingZeroes(reg.ymm.i);

  for (int i = 0; i < 8; i++) {
    uint32_t bits = 0;
    uint32_t item = reg.u32.values[i];

    while (item && (item & 1) == 0) {
      bits++;
      item >>= 1;
    };

    assert(bits == ret.u32.values[i]);
  }
}

__m256i Triggers8x32(__m256i frameBlock, uint32_t prevFrame,
                     uint32_t deltaFrames) {
  const uint32_t lsh = 32 - prevFrame - deltaFrames;
  __m256i highCut = _mm256_slli_epi32(frameBlock, lsh);
  __m256i lowCut = _mm256_srli_epi32(highCut, lsh + prevFrame);
  return NumBits(lowCut);
}

__m128i Triggers4x64(__m256i frameBlock, uint32_t prevFrame,
                     uint32_t deltaFrames) {
  const uint32_t lsh = 64 - prevFrame - deltaFrames;
  __m256i highCut = _mm256_slli_epi64(frameBlock, lsh);
  __m256i lowCut = _mm256_srli_epi64(highCut, lsh + prevFrame);
  __m256i numBitsQuads = NumBits(lowCut);
  __m256i numBits = _mm256_hadd_epi32(numBits, numBits);
  static const __m256i PM_MASK = _mm256_set_epi32(0, 0, 0, 0, 5, 4, 1, 0);

  return _mm256_castsi256_si128(_mm256_permutevar8x32_epi32(numBits, PM_MASK));
}

CacheLine BlockDelta8x32(__m256i frameBlock, __m256i blockOffsets,
                         uint32_t localFrame, float localDelta) {
  __m256i lowerPart = _mm256_slli_epi32(frameBlock, 32 - localFrame);
  __m256i numFrames = NumBits(lowerPart);
  __m256i upperPart = _mm256_srli_epi32(frameBlock, localFrame);
  __m256i nextFrame = NumLeadingZeroes(upperPart);
  const __m256i localFrameV = _mm256_set1_epi32(localFrame + 1);
  const __m256i localFrameV0 = _mm256_set1_epi32(localFrame);
  nextFrame = _mm256_add_epi32(nextFrame, localFrameV);
  __m256i lowBound = Log2(_mm256_srli_epi32(lowerPart, 31 - localFrame));
  CacheLine retVal;
  retVal.a[0].i = _mm256_add_epi32(numFrames, blockOffsets);

  __m256 blockSizeRcp =
      _mm256_rcp_ps(_mm256_cvtepi32_ps(_mm256_sub_epi32(nextFrame, lowBound)));
  __m256 delta = _mm256_mul_ps(
      _mm256_cvtepi32_ps(_mm256_sub_epi32(localFrameV0, lowBound)),
      blockSizeRcp);
  __m256 globalDelta = _mm256_mul_ps(_mm256_set1_ps(localDelta), blockSizeRcp);
  retVal.a[1].f = _mm256_add_ps(delta, globalDelta);

  return retVal;
}

CacheLine BlockDelta8x8(__m256i frameBlock, uint64_t blockOffsets,
                        uint32_t localFrame, float localDelta) {
  return BlockDelta8x32(frameBlock,
                        _mm256_cvtepu8_epi32(_mm_set_epi64x(0, blockOffsets)),
                        localFrame, localDelta);
}

CacheLine BlockDelta8x16(__m256i frameBlock, __m128i blockOffsets,
                         uint32_t localFrame, float localDelta) {
  return BlockDelta8x32(frameBlock, _mm256_cvtepu16_epi32(blockOffsets),
                        localFrame, localDelta);
}

uint32_t NumBits(uint32_t item) {
  uint32_t retVal = 0;
  do {
    retVal += item & 1;
  } while (item >>= 1);

  return retVal;
}

uint32_t NumZeroes(uint32_t item) {
  uint32_t retVal = 0;
  while (item && (item & 1) == 0) {
    retVal++;
    item >>= 1;
  }

  return retVal;
}

struct Frame {
  uint16_t begin;
  uint8_t delta;
  uint8_t blockSize;
};

Frame SingleBlock(uint32_t frameBlock, uint32_t offset, uint32_t localFrame) {
  uint32_t lowerPart = uint64_t(frameBlock) << (32 - localFrame);
  uint32_t numFrames = NumBits(lowerPart);
  uint32_t upperPart = uint64_t(frameBlock) >> (localFrame);
  uint32_t nextFrame = NumZeroes(upperPart) + localFrame + 1;
  uint32_t lInput = uint64_t(lowerPart) >> (31 - localFrame);
  uint32_t lowBound = log2(lInput);
  uint16_t blockOffset = numFrames + offset;
  uint8_t delta = localFrame - lowBound;
  uint8_t blockSize = nextFrame - lowBound;

  return {blockOffset, delta, blockSize};
}

#include <algorithm>
#include <span>

void TestBlockDelta8x8() {
  AsReg reg;
  reg.u32 = {
      0b1011'1101'0110'0010'1001'0010'1000'0000,
      0b1100'0010'0110'0110'0111'1111'1001'1001,
      0b1101'1101'0000'0111'0010'0110'0011'0001,
      0b1010'1110'0100'1111'1001'1001'0010'0011,
      0b1110'0010'1000'1001'0101'0011'1100'1010,
      0b1111'0001'1111'0010'1001'1000'1100'1110,
      0b1110'0001'0100'1010'1111'0010'1111'0111,
      0b1000'0000'0000'0000'0000'0000'0000'0000,
  };

  TsReg offsets;
  offsets.u8 = {7, 6, 2, 4, 5, 6, 7, 8};

  /*int b = 1;

  uint8_t expectedBlockSizes[]{8, 2, 3, 3, 2, 4, 1, 2, 2, 1, 1, 1, 2};

  for (int f = 0; f < 32; f++) {
    auto n = SingleBlock(reg.u32.values[b], offsets.u8.values[b], f);
  }*/

  uint8_t frames0[]{7, 15, 17, 20, 23, 25, 29, 30, 32, 34, 35, 36, 37, 39};
  uint8_t frames1[]{6,  7,  10, 11, 14, 15, 16, 17, 18, 19,
                    20, 21, 24, 25, 28, 29, 32, 37, 38};
  uint8_t frames2[]{2, 3, 7, 8, 12, 13, 16, 19, 20, 21, 27, 29, 30, 31, 33, 34};
  uint8_t frames3[]{4,  5,  6,  10, 13, 16, 17, 20, 21,
                    22, 23, 24, 27, 30, 31, 32, 34, 36};
  uint8_t frames4[]{5,  7,  9,  12, 13, 14, 15, 18,
                    20, 22, 25, 29, 31, 35, 36, 37};
  uint8_t frames5[]{6,  8,  9,  10, 13, 14, 18, 19, 22, 24,
                    27, 28, 29, 30, 31, 35, 36, 37, 38};
  uint8_t frames6[]{7,  8,  9,  10, 12, 13, 14, 15, 17, 20,
                    21, 22, 23, 25, 27, 30, 32, 37, 38, 39};
  uint8_t frames7[]{8, 40};

  std::span<uint8_t> frames[]{frames0, frames1, frames2, frames3,
                              frames4, frames5, frames6, frames7};

  /*uint64_t f = 0;

  for (auto i : frames7) {
    f |= 1ULL << (i - 8);
  }

  f >>= 1;*/

  const float delta = 1 / 120.f;
  const float frameRate = 30;
  const float frameRateRcp = 1 / 30.f;
  const float timeEnd = 32 * frameRateRcp;

  for (float t = 0; t <= timeEnd; t += delta) {
    /*const float frameDelta = t * frameRate;
    const uint8_t frame = frameDelta;
    uint8_t *lowFrame =
        std::upper_bound(std::begin(frames0), std::end(frames0), frame) - 1;
    const float localFrame = frameDelta - *lowFrame;
    const float frameSize = lowFrame[1] - lowFrame[0];
    const float lerpDelta = localFrame / frameSize;*/

    const float frameDeltaLocal = t * frameRate;
    const uint8_t frameLocal = frameDeltaLocal;
    const float deltaLocal = frameDeltaLocal - frameLocal;

    CacheLine line =
        BlockDelta8x8(reg.ymm.i, offsets.u64.values[0], frameLocal, deltaLocal);

    AsReg rBlocks;
    rBlocks.ymm = line.a[0];
    AsReg deltas;
    deltas.ymm = line.a[1];

    for (int b = 0; b < 8; b++) {
      std::span<uint8_t> bSpan = frames[b];
      const float frameDelta = (t * frameRate) + bSpan.front();

      const uint16_t block = rBlocks.u32.values[b] - offsets.u8.values[b];
      const float totalDelta = deltas.f.values[b];

      const float sampledValue =
          (bSpan[block + 1] - bSpan[block]) * totalDelta + bSpan[block];

      const float diff = abs(sampledValue - frameDelta);
      assert(diff < 0.01);

      printf("%u %f %f %f\n", block, frameDelta, sampledValue, totalDelta);
    }
  }
}

#endif

void BenchBlockDelta8x8_() {
  uint32_t frames[8]{1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t offsets[8]{0, 1, 2, 3, 4, 5, 6, 7};

  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] volatile auto item = BlockDelta8x8(frames, offsets, 28);
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "BlockDelta8x8");
  printf("[BlockDelta8x8] Average duration: %zu ns\n", micros / 20'000'000);
}

void BenchBlockDelta8x8() {
  AsReg frames;
  frames.d[0].u32 = {1, 2, 3, 4};
  frames.d[1].u32 = {5, 6, 7, 8};
  TsReg offsets;
  offsets.u16 = {0, 1, 2, 3, 4, 5, 6, 7};
  size_t micros = 0;
  HistogramU h{};

  for (size_t i = 0; i < 20'000'000; i++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] volatile auto item =
#ifdef __AVX__
        BlockDelta8x8(frames.ymm.i, offsets.u64.values[0], 28, 0.5);
#else
        BlockDelta8x8(frames.d[0].xmm.i, offsets.u64.values[0], 28, 0.5);
#endif
    auto dur = std::chrono::high_resolution_clock::now() - startTime;
    const size_t cnt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
    micros += cnt;
    Observe(h, cnt);
  }

  Print(h, "BlockDelta8x8");
  printf("[BlockDelta8x8] Average duration: %zu ns\n", micros / 20'000'000);
}

void Decompose(int type, const void *blockData, uint32_t blockIndex,
               uint32_t numBlocks, float delta, const CacheLine *masks,
               const CacheLine *scales, const CacheLine *offsets,
               float *poseTable, uint32_t *ptIndices) {
  CacheLine line[4];

  switch (type) {
  case 0: {
    const __m128i *firstBlock =
        static_cast<const __m128i *>(blockData) + blockIndex * 2;
    const __m128i *nextBlock = firstBlock + (blockIndex % numBlocks) * 2;

    Decompose16x16b(firstBlock, line[0]);
    Decompose16x16b(nextBlock, line[1]);

    line[0].a[0].f = _mm256_cvtepi32_ps(line[0].a[0].i);
    line[0].a[1].f = _mm256_cvtepi32_ps(line[0].a[1].i);
    line[1].a[0].f = _mm256_cvtepi32_ps(line[1].a[0].i);
    line[1].a[1].f = _mm256_cvtepi32_ps(line[1].a[1].i);

    LerpCacheLines(line, delta);

    ConvertCacheline4x4(line[0], *scales, *offsets);
    CacheLineToPoseTable(line[0], poseTable, ptIndices);

    break;
  }
  case 3: {
    const __m128i *firstBlock =
        static_cast<const __m128i *>(blockData) + blockIndex;

    Decompose16x16b(firstBlock, line[0]);

    line[0].a[0].f = _mm256_cvtepi32_ps(line[0].a[0].i);
    line[0].a[1].f = _mm256_cvtepi32_ps(line[0].a[1].i);

    LerpCacheLine(line[0], delta);

    ConvertCacheline4x4(line[0], *scales, *offsets);
    LowCacheLineToPoseTable(line[0], poseTable, ptIndices);

    break;
  }
  case 1: {
    const __m128i *firstBlock =
        static_cast<const __m128i *>(blockData) + blockIndex;
    const __m128i *nextBlock = firstBlock + (blockIndex % numBlocks);

    Decompose4x32b(*firstBlock, line[0]);
    Decompose4x32b(*nextBlock, line[1]);

    ConvertCacheline4x4(line[0], *masks, *scales, *offsets);
    ConvertCacheline4x4(line[1], *masks, *scales, *offsets);

    LerpCacheLines(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices);

    break;
  }
  case 4: {
    const uint64_t *firstBlock =
        static_cast<const uint64_t *>(blockData) + blockIndex;

    __m128i loadedBlock =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(firstBlock));

    Decompose4x32b(loadedBlock, line[0]);

    ConvertCacheline4x4(line[0], *masks, *scales, *offsets);

    LerpCacheLine(line[0], delta);
    LowCacheLineToPoseTable(line[0], poseTable, ptIndices);

    break;
  }
  case 2: {
    const CacheLine *firstBlock =
        static_cast<const CacheLine *>(blockData) + blockIndex;
    const CacheLine *nextBlock = firstBlock + (blockIndex % numBlocks);
    Decompose4x16bLow(*firstBlock, line);
    Decompose4x16bLow(*nextBlock, line + 2);

    ConvertCacheline4x4(line[0], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[1], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[2], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[3], masks[0], scales[0], offsets[0]);

    LerpCacheLines2(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices);
    CacheLineToPoseTable(line[1], poseTable, ptIndices + 16);

    Decompose4x16bMid(*firstBlock, line);
    Decompose4x16bMid(*nextBlock, line + 2);

    ConvertCacheline4x4(line[0], masks[1], scales[1], offsets[1]);
    ConvertCacheline4x4(line[1], masks[1], scales[1], offsets[1]);
    ConvertCacheline4x4(line[2], masks[1], scales[1], offsets[1]);
    ConvertCacheline4x4(line[3], masks[1], scales[1], offsets[1]);

    LerpCacheLines2(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices + 32);
    CacheLineToPoseTable(line[1], poseTable, ptIndices + 48);

    Decompose4x16bHigh(*firstBlock, line);
    Decompose4x16bHigh(*nextBlock, line + 2);

    ConvertCacheline4x4(line[0], masks[2], scales[2], offsets[2]);
    ConvertCacheline4x4(line[1], masks[2], scales[2], offsets[2]);
    ConvertCacheline4x4(line[2], masks[2], scales[2], offsets[2]);
    ConvertCacheline4x4(line[3], masks[2], scales[2], offsets[2]);

    LerpCacheLines2(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices + 64);
    CacheLineToPoseTable(line[1], poseTable, ptIndices + 80);
    break;
  }
  case 5: {
    const __m128i *firstBlock =
        static_cast<const __m128i *>(blockData) + blockIndex * 2;
    const __m128i *nextBlock = firstBlock + (blockIndex % numBlocks) * 2;
    Decompose2x16bLow(firstBlock, line);
    Decompose2x16bLow(nextBlock, line + 2);

    ConvertCacheline4x4(line[0], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[1], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[2], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[3], masks[0], scales[0], offsets[0]);

    LerpCacheLines2(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices);
    CacheLineToPoseTable(line[1], poseTable, ptIndices + 16);

    Decompose2x16bHigh(firstBlock + 1, line);
    Decompose2x16bHigh(nextBlock + 1, line + 2);

    ConvertCacheline4x4(line[0], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[1], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[2], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[3], masks[0], scales[0], offsets[0]);

    LerpCacheLines2(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices + 32);
    CacheLineToPoseTable(line[1], poseTable, ptIndices + 48);
    break;
  }
  case 6: {
    const __m128i *firstBlock =
        static_cast<const __m128i *>(blockData) + blockIndex * 2;
    Decompose2x16bLow(firstBlock, line);

    ConvertCacheline4x4(line[0], masks[0], scales[0], offsets[0]);
    ConvertCacheline4x4(line[1], masks[0], scales[0], offsets[0]);

    LerpCacheLines(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices);

    Decompose2x16bHigh(firstBlock + 1, line);

    ConvertCacheline4x4(line[0], masks[1], scales[1], offsets[1]);
    ConvertCacheline4x4(line[1], masks[1], scales[1], offsets[1]);

    LerpCacheLines(line, delta);
    CacheLineToPoseTable(line[0], poseTable, ptIndices + 16);
    break;
  }
  }
}

struct NodeMap {
  mutable uint32_t id;
  uint32_t poseTableElementIndex;
};

template <class C> struct Pointer {
  int32_t ptr;

  const C *data() const { return nullptr; }
};

template <class C> struct Array {
  uint32_t numItems;
  Pointer<C> data;

  const C *begin() const { return data.data(); }
  const C *end() const { return begin() + numItems; }
};

struct CurveSet {
  uint32_t type;
  Pointer<CacheLine> offsets;
  Pointer<CacheLine> scales;
  Pointer<CacheLine> masks;
  Pointer<void> data;
  Array<uint32_t> poseTableIndices;
};

struct FrameSet {
  uint32_t type;
  Pointer<void> data;
};

struct CurveBatch {
  Array<NodeMap> userChannels;
  Array<NodeMap> rigNodeTransforms;
  Array<NodeMap> rigNodeScales;
  Array<CurveSet> sets;
  Array<FrameSet> frameSets;
  Array<FrameSet> triggers;
};

struct SkeletonNode {
  uint32_t id;
  uint16_t index;
  uint16_t numChildren;
};

struct Skeleton {
  Array<SkeletonNode> rigNodes;
  Array<float> rigNodeTransforms;
  Array<SkeletonNode> rigScaleNodes;
  Array<float> rigNodeScales;
};

#include <map>
#include <vector>

struct SimpleSkeletonBind {
  
};

struct CurveBatchToSkeletonBinding {
  float *globalPose;
  float *userChannels;
  uint8_t *triggerTable;
};

struct AnimationMachine {
  Skeleton *skeleton;
  void *machineBuffer;
  // float *userChannels;
  uint8_t *triggerTable;
  std::vector<float *> binds;
  std::vector<CurveBatch *> batches;
  std::map<uint32_t, uint32_t> userChannels;
};

void *CreateMachineBuffer(AnimationMachine &item) {
  const uint32_t numBoneTms = item.skeleton->rigNodeTransforms.numItems;
  const uint32_t numBoneScales = item.skeleton->rigNodeScales.numItems;

  for (auto &b : item.batches) {
    for (auto &u : b->userChannels) {
      u.id = item.userChannels.try_emplace(u.id, item.userChannels.size())
                 .first->second;
    }
  }
}

int main() {
  /*TestDecompose4x32b();
  TestDecompose16x16b();
  TestDecompose2AReg8x4();
  SmokeTestQuatMult();
  SmokeTestDualQuatMult();
  SmokeTestComputeQuatReal();
  TestComputeQuatRealZero();*/

  TestLog2();
  TestNumBits();
  TestNumLeadingZeroes();
  TestBlockDelta8x8();

  BenchQuatMult(); // sse 17, avx 17
  // BenchDualMult();        // sse 691 avx 683
  BenchComputeQuatReal(); // sse 17

  Bench4x32b();    // sse 17
  Bench16x16b();   // sse 17, avx 17
  Bench2AReg8x4(); // sse 18, avx 17

  BenchBlockDelta8x8();

  return 0;
}
