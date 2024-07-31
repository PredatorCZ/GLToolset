#pragma once
constexpr bool isInstanced = false;
#ifdef __cplusplus
#define __glslver__ 450

#define _cint_ int
#define _cfloat_ float
#define _cdouble_ double

template <class S> union vec2_;
template <class S> union vec3_;
template <class S> union vec4_;

template <class C, class S> struct vecx_ {
  C operator++(int);
  C operator--(int);
  C operator-();
  C operator+(C);
  C operator-(C);
  C operator*(C);
  C operator/(C);
  bool operator<(C);
  bool operator>(C);
  bool operator<=(C);
  bool operator>=(C);
  bool operator==(C);
  bool operator!=(C);
  bool operator&&(C);
  bool operator||(C);
  C operator>>(C);
  C operator<<(C);
  C operator&(C);
  C operator|(C);
  C operator^(C);
  C operator>>(S);
  C operator<<(S);
  C operator&(S);
  C operator|(S);
  C operator^(S);
  C operator=(C);
  C operator+=(C);
  C operator-=(C);
  C operator*=(C);
  C operator/=(C);
  C operator>>=(C);
  C operator<<=(C);
  C operator&=(C);
  C operator|=(C);
  C operator^=(C);
  C operator+(S);
  C operator-(S);
  C operator*(S);
  C operator/(S);
  C operator+=(S);
  C operator-=(S);
  C operator*=(S);
  C operator/=(S);
  C operator>>=(S);
  C operator<<=(S);
  C operator&=(S);
  C operator|=(S);
  C operator^=(S);
  operator C() const;
};

template <class C> union scalar_ {
  using base_type = C;
  constexpr scalar_(){};
  constexpr scalar_(C x) : _x_(x) {}
  constexpr scalar_(const scalar_ &x) : _x_(x._x_) {}
  template <class S> constexpr scalar_(const scalar_<S> &x) : _x_(x._x_) {}
  C _x_;
  vecx_<vec2_<scalar_>, scalar_> xx;
  vecx_<vec3_<scalar_>, scalar_> xxx;
  vecx_<vec4_<scalar_>, scalar_> xxxx;

  constexpr operator C() const { return _x_; }

  scalar_ operator++(int);
  scalar_ operator--(int);
  scalar_ operator-() const;
  // scalar_ operator+(scalar_) const;
  // scalar_ operator-(scalar_) const;
  // explicit scalar_ operator*(scalar_) const;
  // explicit scalar_ operator/(scalar_) const;
  // bool operator<(scalar_) const;
  // bool operator>(scalar_) const;
  // bool operator<=(scalar_) const;
  // bool operator>=(scalar_) const;
  constexpr bool operator==(scalar_ o) const { return o._x_ == _x_; }
  constexpr bool operator!=(scalar_ o) const { return o._x_ != _x_; }
  constexpr bool operator&&(scalar_ o) const { return o._x_ && _x_; }
  constexpr bool operator||(scalar_ o) const { return o._x_ || _x_; }
  constexpr scalar_ &operator=(scalar_ o) {
    _x_ = o._x_;
    return *this;
  }
  bool operator^(scalar_) const;
  scalar_ operator+=(scalar_);
  scalar_ operator-=(scalar_);
  scalar_ operator*=(scalar_);
  scalar_ operator/=(scalar_);

  constexpr friend scalar_ operator+(C x, scalar_ o) {
    x += o._x_;
    return x;
  }
  constexpr friend scalar_ operator-(C x, scalar_ o) {
    x -= o._x_;
    return x;
  }
  constexpr friend scalar_ operator*(C x, scalar_ o) {
    x *= o._x_;
    return x;
  }
  constexpr friend scalar_ operator/(C x, scalar_ o) {
    x /= o._x_;
    return x;
  }
  constexpr friend scalar_ operator%(C x, scalar_ o) {
    x %= o._x_;
    return x;
  }
  constexpr friend bool operator<(C x, scalar_ y) {return x < y._x_; }
  constexpr friend bool operator>(C x, scalar_ y) {return x > y._x_; }
  constexpr friend bool operator<=(C x, scalar_ y) {return x <= y._x_; }
  constexpr friend bool operator>=(C x, scalar_ y) {return x >= y._x_; }
  constexpr friend bool operator==(C x, scalar_ y) {return x == y._x_; }
  constexpr friend bool operator!=(C x, scalar_ y) {return x != y._x_; }
  friend bool operator&&(C, scalar_);
  friend bool operator||(C, scalar_);
  friend scalar_ operator+=(C, scalar_);
  friend scalar_ operator-=(C, scalar_);
  friend scalar_ operator*=(C, scalar_);
  friend scalar_ operator/=(C, scalar_);
};

using int_ = scalar_<_cint_>;
using uint = scalar_<unsigned>;
using float_ = scalar_<_cfloat_>;
using double_ = scalar_<_cdouble_>;

template <class S> union vec2_ {
  using scalar_type = S;
  template <class V> using vecx = vecx_<V, S>;
  constexpr vec2_() {}
  constexpr vec2_(S) {}
  constexpr vec2_(S, S) {}
  constexpr vec2_(S::base_type) {}
  constexpr vec2_(const vec2_ &) {}
  template <class OS> constexpr vec2_(const vec2_<OS> &other) {}

  operator S() const;
  S x;
  S y;
  vecx<vec2_> xx;
  vecx<vec2_> xy;
  vecx<vec2_> yx;
  vecx<vec2_> yy;
  vecx<vec3_<S>> xxx;
  vecx<vec3_<S>> xxy;
  vecx<vec3_<S>> xyx;
  vecx<vec3_<S>> xyy;
  vecx<vec3_<S>> yxx;
  vecx<vec3_<S>> yxy;
  vecx<vec3_<S>> yyx;
  vecx<vec3_<S>> yyy;
  vecx<vec4_<S>> xxxx;
  vecx<vec4_<S>> xxxy;
  vecx<vec4_<S>> xxyx;
  vecx<vec4_<S>> xxyy;
  vecx<vec4_<S>> xyxx;
  vecx<vec4_<S>> xyxy;
  vecx<vec4_<S>> xyyx;
  vecx<vec4_<S>> xyyy;
  vecx<vec4_<S>> yxxx;
  vecx<vec4_<S>> yxxy;
  vecx<vec4_<S>> yxyx;
  vecx<vec4_<S>> yxyy;
  vecx<vec4_<S>> yyxx;
  vecx<vec4_<S>> yyxy;
  vecx<vec4_<S>> yyyx;
  vecx<vec4_<S>> yyyy;

  vec2_ operator++(int);
  vec2_ operator--(int);
  vec2_ operator-() const;
  vec2_ operator+(vec2_) const;
  vec2_ operator-(vec2_) const;
  vec2_ operator*(vec2_) const;
  // template <class vec> vec2_ operator*(matx<vec>);
  vec2_ operator/(vec2_) const;
  vec2_ operator>>(vec2_) const;
  vec2_ operator<<(vec2_) const;
  vec2_ operator|(vec2_) const;
  vec2_ operator&(vec2_) const;
  vec2_ operator^(vec2_) const;
  bool operator<(vec2_) const;
  bool operator>(vec2_) const;
  bool operator<=(vec2_) const;
  bool operator>=(vec2_) const;
  bool operator==(vec2_) const;
  bool operator!=(vec2_) const;
  bool operator&&(vec2_) const;
  bool operator^(vec2_) const;
  bool operator||(vec2_) const;
  vec2_ operator=(vec2_);
  vec2_ operator+=(vec2_);
  vec2_ operator-=(vec2_);
  vec2_ operator*=(vec2_);
  vec2_ operator/=(vec2_);
  vec2_ operator>>=(vec2_);
  vec2_ operator<<=(vec2_);
  vec2_ operator|=(vec2_);
  vec2_ operator&=(vec2_);
  vec2_ operator^=(vec2_);

  vec2_ operator+(S) const;
  vec2_ operator-(S) const;
  vec2_ operator*(S) const;
  vec2_ operator/(S) const;
  vec2_ operator>>(S) const;
  vec2_ operator<<(S) const;
  vec2_ operator|(S) const;
  vec2_ operator&(S) const;
  vec2_ operator^(S) const;
  vec2_ operator+=(S);
  vec2_ operator-=(S);
  vec2_ operator*=(S);
  vec2_ operator/=(S);
  vec2_ operator>>=(S);
  vec2_ operator<<=(S);
  vec2_ operator|=(S);
  vec2_ operator&=(S);
  vec2_ operator^=(S);
  vec2_ operator=(S);

  vec2_ operator+(S::base_type) const;
  vec2_ operator-(S::base_type) const;
  vec2_ operator*(S::base_type) const;
  vec2_ operator/(S::base_type) const;
  vec2_ operator>>(S::base_type) const;
  vec2_ operator<<(S::base_type) const;
  vec2_ operator|(S::base_type) const;
  vec2_ operator&(S::base_type) const;
  vec2_ operator^(S::base_type) const;
  vec2_ operator+=(S::base_type);
  vec2_ operator-=(S::base_type);
  vec2_ operator*=(S::base_type);
  vec2_ operator/=(S::base_type);
  vec2_ operator>>=(S::base_type);
  vec2_ operator<<=(S::base_type);
  vec2_ operator|=(S::base_type);
  vec2_ operator&=(S::base_type);
  vec2_ operator^=(S::base_type);
};

template <class S> union vec3_ {
  using scalar_type = S;
  template <class V> using vecx = vecx_<V, S>;
  constexpr vec3_() {}
  constexpr vec3_(const vec3_ &) {}
  constexpr vec3_(S s) : x(s), y(s), z(s) {}
  constexpr vec3_(S x_, S y_, S z_) : x(x_), y(y_), z(z_) {}
  constexpr vec3_(vec2_<S>, S) {}
  constexpr vec3_(S, vec2_<S>) {}
  constexpr vec3_(S::base_type s) : x(s), y(s), z(s) {}

  operator S() const;
  operator vec2_<S>() const;

  vec3_ operator=(vec3_);
  vec3_ operator=(S);

  struct {
    S x;
    S y;
    S z;
  };
  vec2_<S> xx;
  vec2_<S> xy;
  vec2_<S> xz;
  vec2_<S> yx;
  vec2_<S> yy;
  vec2_<S> yz;
  vec2_<S> zx;
  vec2_<S> zy;
  vec2_<S> zz;
  vecx<vec3_> xxx;
  vecx<vec3_> xxy;
  vecx<vec3_> xxz;
  vecx<vec3_> xyx;
  vecx<vec3_> xyy;
  vecx<vec3_> xyz;
  vecx<vec3_> xzx;
  vecx<vec3_> xzy;
  vecx<vec3_> xzz;
  vecx<vec3_> yxx;
  vecx<vec3_> yxy;
  vecx<vec3_> yxz;
  vecx<vec3_> yyx;
  vecx<vec3_> yyy;
  vecx<vec3_> yyz;
  vecx<vec3_> yzx;
  vecx<vec3_> yzy;
  vecx<vec3_> yzz;
  vecx<vec3_> zxx;
  vecx<vec3_> zxy;
  vecx<vec3_> zxz;
  vecx<vec3_> zyx;
  vecx<vec3_> zyy;
  vecx<vec3_> zyz;
  vecx<vec3_> zzx;
  vecx<vec3_> zzy;
  vecx<vec3_> zzz;
  vecx<vec4_<S>> xxxx;
  vecx<vec4_<S>> xxxy;
  vecx<vec4_<S>> xxxz;
  vecx<vec4_<S>> xxyx;
  vecx<vec4_<S>> xxyy;
  vecx<vec4_<S>> xxyz;
  vecx<vec4_<S>> xxzx;
  vecx<vec4_<S>> xxzy;
  vecx<vec4_<S>> xxzz;
  vecx<vec4_<S>> xyxx;
  vecx<vec4_<S>> xyxy;
  vecx<vec4_<S>> xyxz;
  vecx<vec4_<S>> xyyx;
  vecx<vec4_<S>> xyyy;
  vecx<vec4_<S>> xyyz;
  vecx<vec4_<S>> xyzx;
  vecx<vec4_<S>> xyzy;
  vecx<vec4_<S>> xyzz;
  vecx<vec4_<S>> xzxx;
  vecx<vec4_<S>> xzxy;
  vecx<vec4_<S>> xzxz;
  vecx<vec4_<S>> xzyx;
  vecx<vec4_<S>> xzyy;
  vecx<vec4_<S>> xzyz;
  vecx<vec4_<S>> xzzx;
  vecx<vec4_<S>> xzzy;
  vecx<vec4_<S>> xzzz;
  vecx<vec4_<S>> yxxx;
  vecx<vec4_<S>> yxxy;
  vecx<vec4_<S>> yxxz;
  vecx<vec4_<S>> yxyx;
  vecx<vec4_<S>> yxyy;
  vecx<vec4_<S>> yxyz;
  vecx<vec4_<S>> yxzx;
  vecx<vec4_<S>> yxzy;
  vecx<vec4_<S>> yxzz;
  vecx<vec4_<S>> yyxx;
  vecx<vec4_<S>> yyxy;
  vecx<vec4_<S>> yyxz;
  vecx<vec4_<S>> yyyx;
  vecx<vec4_<S>> yyyy;
  vecx<vec4_<S>> yyyz;
  vecx<vec4_<S>> yyzx;
  vecx<vec4_<S>> yyzy;
  vecx<vec4_<S>> yyzz;
  vecx<vec4_<S>> yzxx;
  vecx<vec4_<S>> yzxy;
  vecx<vec4_<S>> yzxz;
  vecx<vec4_<S>> yzyx;
  vecx<vec4_<S>> yzyy;
  vecx<vec4_<S>> yzyz;
  vecx<vec4_<S>> yzzx;
  vecx<vec4_<S>> yzzy;
  vecx<vec4_<S>> yzzz;
  vecx<vec4_<S>> zxxx;
  vecx<vec4_<S>> zxxy;
  vecx<vec4_<S>> zxxz;
  vecx<vec4_<S>> zxyx;
  vecx<vec4_<S>> zxyy;
  vecx<vec4_<S>> zxyz;
  vecx<vec4_<S>> zxzx;
  vecx<vec4_<S>> zxzy;
  vecx<vec4_<S>> zxzz;
  vecx<vec4_<S>> zyxx;
  vecx<vec4_<S>> zyxy;
  vecx<vec4_<S>> zyxz;
  vecx<vec4_<S>> zyyx;
  vecx<vec4_<S>> zyyy;
  vecx<vec4_<S>> zyyz;
  vecx<vec4_<S>> zyzx;
  vecx<vec4_<S>> zyzy;
  vecx<vec4_<S>> zyzz;
  vecx<vec4_<S>> zzxx;
  vecx<vec4_<S>> zzxy;
  vecx<vec4_<S>> zzxz;
  vecx<vec4_<S>> zzyx;
  vecx<vec4_<S>> zzyy;
  vecx<vec4_<S>> zzyz;
  vecx<vec4_<S>> zzzx;
  vecx<vec4_<S>> zzzy;
  vecx<vec4_<S>> zzzz;

  vec3_ operator++(int);
  vec3_ operator--(int);
  vec3_ operator-() const;
  vec3_ operator+(vec3_) const;
  vec3_ operator-(vec3_) const;
  vec3_ operator*(vec3_) const;
  // template <class vec> vec3 operator*(matx<vec>);
  vec3_ operator/(vec3_) const;
  vec3_ operator>>(vec3_) const;
  vec3_ operator<<(vec3_) const;
  vec3_ operator|(vec3_) const;
  vec3_ operator&(vec3_) const;
  vec3_ operator^(vec3_) const;
  bool operator<(vec3_) const;
  bool operator>(vec3_) const;
  bool operator<=(vec3_) const;
  bool operator>=(vec3_) const;
  bool operator==(vec3_) const;
  bool operator!=(vec3_) const;
  bool operator&&(vec3_) const;
  bool operator^(vec3_) const;
  bool operator||(vec3_) const;
  vec3_ operator+=(vec3_);
  vec3_ operator-=(vec3_);
  vec3_ operator*=(vec3_);
  vec3_ operator/=(vec3_);
  vec3_ operator>>=(vec3_);
  vec3_ operator<<=(vec3_);
  vec3_ operator|=(vec3_);
  vec3_ operator&=(vec3_);
  vec3_ operator^=(vec3_);

  vec3_ operator+(S) const;
  vec3_ operator-(S) const;
  vec3_ operator*(S) const;
  vec3_ operator/(S) const;
  vec3_ operator>>(S) const;
  vec3_ operator<<(S) const;
  vec3_ operator|(S) const;
  vec3_ operator&(S) const;
  vec3_ operator^(S) const;
  vec3_ operator+=(S);
  vec3_ operator-=(S);
  vec3_ operator*=(S);
  vec3_ operator>>=(S);
  vec3_ operator<<=(S);
  vec3_ operator|=(S);
  vec3_ operator&=(S);
  vec3_ operator^=(S);
  vec3_ operator/=(S);

  vec3_ operator+(S::base_type) const;
  vec3_ operator-(S::base_type) const;
  vec3_ operator*(S::base_type) const;
  vec3_ operator/(S::base_type) const;
  vec3_ operator>>(S::base_type) const;
  vec3_ operator<<(S::base_type) const;
  vec3_ operator|(S::base_type) const;
  vec3_ operator&(S::base_type) const;
  vec3_ operator^(S::base_type) const;
  vec3_ operator+=(S::base_type);
  vec3_ operator-=(S::base_type);
  vec3_ operator*=(S::base_type);
  vec3_ operator/=(S::base_type);
  vec3_ operator>>=(S::base_type);
  vec3_ operator<<=(S::base_type);
  vec3_ operator|=(S::base_type);
  vec3_ operator&=(S::base_type);
  vec3_ operator^=(S::base_type);
};

template <class S> union vec4_ {
  using scalar_type = S;
  template <class V> using vecx = vecx_<V, S>;
  constexpr vec4_() {}
  constexpr vec4_(const vec4_ &) {}
  constexpr vec4_(S) {}
  constexpr vec4_(S, S, S, S) {}
  constexpr vec4_(vec3_<S>, S) {}
  constexpr vec4_(S, vec3_<S>) {}
  constexpr vec4_(vec2_<S>, S, S) {}
  constexpr vec4_(S, vec2_<S>, S) {}
  constexpr vec4_(S, S, vec2_<S>) {}
  constexpr vec4_(S::base_type) {}

  operator S();
  operator vec2_<S>();
  operator vec3_<S>();

  S w;
  S x;
  S y;
  S z;
  vec2_<S> ww;
  vec2_<S> wx;
  vec2_<S> wy;
  vec2_<S> wz;
  vec2_<S> xw;
  vec2_<S> xx;
  vec2_<S> xy;
  vec2_<S> xz;
  vec2_<S> yw;
  vec2_<S> yx;
  vec2_<S> yy;
  vec2_<S> yz;
  vec2_<S> zw;
  vec2_<S> zx;
  vec2_<S> zy;
  vec2_<S> zz;
  vec3_<S> www;
  vec3_<S> wwx;
  vec3_<S> wwy;
  vec3_<S> wwz;
  vec3_<S> wxw;
  vec3_<S> wxx;
  vec3_<S> wxy;
  vec3_<S> wxz;
  vec3_<S> wyw;
  vec3_<S> wyx;
  vec3_<S> wyy;
  vec3_<S> wyz;
  vec3_<S> wzw;
  vec3_<S> wzx;
  vec3_<S> wzy;
  vec3_<S> wzz;
  vec3_<S> xww;
  vec3_<S> xwx;
  vec3_<S> xwy;
  vec3_<S> xwz;
  vec3_<S> xxw;
  vec3_<S> xxx;
  vec3_<S> xxy;
  vec3_<S> xxz;
  vec3_<S> xyw;
  vec3_<S> xyx;
  vec3_<S> xyy;
  vec3_<S> xyz;
  vec3_<S> xzw;
  vec3_<S> xzx;
  vec3_<S> xzy;
  vec3_<S> xzz;
  vec3_<S> yww;
  vec3_<S> ywx;
  vec3_<S> ywy;
  vec3_<S> ywz;
  vec3_<S> yxw;
  vec3_<S> yxx;
  vec3_<S> yxy;
  vec3_<S> yxz;
  vec3_<S> yyw;
  vec3_<S> yyx;
  vec3_<S> yyy;
  vec3_<S> yyz;
  vec3_<S> yzw;
  vec3_<S> yzx;
  vec3_<S> yzy;
  vec3_<S> yzz;
  vec3_<S> zww;
  vec3_<S> zwx;
  vec3_<S> zwy;
  vec3_<S> zwz;
  vec3_<S> zxw;
  vec3_<S> zxx;
  vec3_<S> zxy;
  vec3_<S> zxz;
  vec3_<S> zyw;
  vec3_<S> zyx;
  vec3_<S> zyy;
  vec3_<S> zyz;
  vec3_<S> zzw;
  vec3_<S> zzx;
  vec3_<S> zzy;
  vec3_<S> zzz;
  vecx<vec4_> wwww;
  vecx<vec4_> wwwx;
  vecx<vec4_> wwwy;
  vecx<vec4_> wwwz;
  vecx<vec4_> wwxw;
  vecx<vec4_> wwxx;
  vecx<vec4_> wwxy;
  vecx<vec4_> wwxz;
  vecx<vec4_> wwyw;
  vecx<vec4_> wwyx;
  vecx<vec4_> wwyy;
  vecx<vec4_> wwyz;
  vecx<vec4_> wwzw;
  vecx<vec4_> wwzx;
  vecx<vec4_> wwzy;
  vecx<vec4_> wwzz;
  vecx<vec4_> wxww;
  vecx<vec4_> wxwx;
  vecx<vec4_> wxwy;
  vecx<vec4_> wxwz;
  vecx<vec4_> wxxw;
  vecx<vec4_> wxxx;
  vecx<vec4_> wxxy;
  vecx<vec4_> wxxz;
  vecx<vec4_> wxyw;
  vecx<vec4_> wxyx;
  vecx<vec4_> wxyy;
  vecx<vec4_> wxyz;
  vecx<vec4_> wxzw;
  vecx<vec4_> wxzx;
  vecx<vec4_> wxzy;
  vecx<vec4_> wxzz;
  vecx<vec4_> wyww;
  vecx<vec4_> wywx;
  vecx<vec4_> wywy;
  vecx<vec4_> wywz;
  vecx<vec4_> wyxw;
  vecx<vec4_> wyxx;
  vecx<vec4_> wyxy;
  vecx<vec4_> wyxz;
  vecx<vec4_> wyyw;
  vecx<vec4_> wyyx;
  vecx<vec4_> wyyy;
  vecx<vec4_> wyyz;
  vecx<vec4_> wyzw;
  vecx<vec4_> wyzx;
  vecx<vec4_> wyzy;
  vecx<vec4_> wyzz;
  vecx<vec4_> wzww;
  vecx<vec4_> wzwx;
  vecx<vec4_> wzwy;
  vecx<vec4_> wzwz;
  vecx<vec4_> wzxw;
  vecx<vec4_> wzxx;
  vecx<vec4_> wzxy;
  vecx<vec4_> wzxz;
  vecx<vec4_> wzyw;
  vecx<vec4_> wzyx;
  vecx<vec4_> wzyy;
  vecx<vec4_> wzyz;
  vecx<vec4_> wzzw;
  vecx<vec4_> wzzx;
  vecx<vec4_> wzzy;
  vecx<vec4_> wzzz;
  vecx<vec4_> xwww;
  vecx<vec4_> xwwx;
  vecx<vec4_> xwwy;
  vecx<vec4_> xwwz;
  vecx<vec4_> xwxw;
  vecx<vec4_> xwxx;
  vecx<vec4_> xwxy;
  vecx<vec4_> xwxz;
  vecx<vec4_> xwyw;
  vecx<vec4_> xwyx;
  vecx<vec4_> xwyy;
  vecx<vec4_> xwyz;
  vecx<vec4_> xwzw;
  vecx<vec4_> xwzx;
  vecx<vec4_> xwzy;
  vecx<vec4_> xwzz;
  vecx<vec4_> xxww;
  vecx<vec4_> xxwx;
  vecx<vec4_> xxwy;
  vecx<vec4_> xxwz;
  vecx<vec4_> xxxw;
  vecx<vec4_> xxxx;
  vecx<vec4_> xxxy;
  vecx<vec4_> xxxz;
  vecx<vec4_> xxyw;
  vecx<vec4_> xxyx;
  vecx<vec4_> xxyy;
  vecx<vec4_> xxyz;
  vecx<vec4_> xxzw;
  vecx<vec4_> xxzx;
  vecx<vec4_> xxzy;
  vecx<vec4_> xxzz;
  vecx<vec4_> xyww;
  vecx<vec4_> xywx;
  vecx<vec4_> xywy;
  vecx<vec4_> xywz;
  vecx<vec4_> xyxw;
  vecx<vec4_> xyxx;
  vecx<vec4_> xyxy;
  vecx<vec4_> xyxz;
  vecx<vec4_> xyyw;
  vecx<vec4_> xyyx;
  vecx<vec4_> xyyy;
  vecx<vec4_> xyyz;
  vecx<vec4_> xyzw;
  vecx<vec4_> xyzx;
  vecx<vec4_> xyzy;
  vecx<vec4_> xyzz;
  vecx<vec4_> xzww;
  vecx<vec4_> xzwx;
  vecx<vec4_> xzwy;
  vecx<vec4_> xzwz;
  vecx<vec4_> xzxw;
  vecx<vec4_> xzxx;
  vecx<vec4_> xzxy;
  vecx<vec4_> xzxz;
  vecx<vec4_> xzyw;
  vecx<vec4_> xzyx;
  vecx<vec4_> xzyy;
  vecx<vec4_> xzyz;
  vecx<vec4_> xzzw;
  vecx<vec4_> xzzx;
  vecx<vec4_> xzzy;
  vecx<vec4_> xzzz;
  vecx<vec4_> ywww;
  vecx<vec4_> ywwx;
  vecx<vec4_> ywwy;
  vecx<vec4_> ywwz;
  vecx<vec4_> ywxw;
  vecx<vec4_> ywxx;
  vecx<vec4_> ywxy;
  vecx<vec4_> ywxz;
  vecx<vec4_> ywyw;
  vecx<vec4_> ywyx;
  vecx<vec4_> ywyy;
  vecx<vec4_> ywyz;
  vecx<vec4_> ywzw;
  vecx<vec4_> ywzx;
  vecx<vec4_> ywzy;
  vecx<vec4_> ywzz;
  vecx<vec4_> yxww;
  vecx<vec4_> yxwx;
  vecx<vec4_> yxwy;
  vecx<vec4_> yxwz;
  vecx<vec4_> yxxw;
  vecx<vec4_> yxxx;
  vecx<vec4_> yxxy;
  vecx<vec4_> yxxz;
  vecx<vec4_> yxyw;
  vecx<vec4_> yxyx;
  vecx<vec4_> yxyy;
  vecx<vec4_> yxyz;
  vecx<vec4_> yxzw;
  vecx<vec4_> yxzx;
  vecx<vec4_> yxzy;
  vecx<vec4_> yxzz;
  vecx<vec4_> yyww;
  vecx<vec4_> yywx;
  vecx<vec4_> yywy;
  vecx<vec4_> yywz;
  vecx<vec4_> yyxw;
  vecx<vec4_> yyxx;
  vecx<vec4_> yyxy;
  vecx<vec4_> yyxz;
  vecx<vec4_> yyyw;
  vecx<vec4_> yyyx;
  vecx<vec4_> yyyy;
  vecx<vec4_> yyyz;
  vecx<vec4_> yyzw;
  vecx<vec4_> yyzx;
  vecx<vec4_> yyzy;
  vecx<vec4_> yyzz;
  vecx<vec4_> yzww;
  vecx<vec4_> yzwx;
  vecx<vec4_> yzwy;
  vecx<vec4_> yzwz;
  vecx<vec4_> yzxw;
  vecx<vec4_> yzxx;
  vecx<vec4_> yzxy;
  vecx<vec4_> yzxz;
  vecx<vec4_> yzyw;
  vecx<vec4_> yzyx;
  vecx<vec4_> yzyy;
  vecx<vec4_> yzyz;
  vecx<vec4_> yzzw;
  vecx<vec4_> yzzx;
  vecx<vec4_> yzzy;
  vecx<vec4_> yzzz;
  vecx<vec4_> zwww;
  vecx<vec4_> zwwx;
  vecx<vec4_> zwwy;
  vecx<vec4_> zwwz;
  vecx<vec4_> zwxw;
  vecx<vec4_> zwxx;
  vecx<vec4_> zwxy;
  vecx<vec4_> zwxz;
  vecx<vec4_> zwyw;
  vecx<vec4_> zwyx;
  vecx<vec4_> zwyy;
  vecx<vec4_> zwyz;
  vecx<vec4_> zwzw;
  vecx<vec4_> zwzx;
  vecx<vec4_> zwzy;
  vecx<vec4_> zwzz;
  vecx<vec4_> zxww;
  vecx<vec4_> zxwx;
  vecx<vec4_> zxwy;
  vecx<vec4_> zxwz;
  vecx<vec4_> zxxw;
  vecx<vec4_> zxxx;
  vecx<vec4_> zxxy;
  vecx<vec4_> zxxz;
  vecx<vec4_> zxyw;
  vecx<vec4_> zxyx;
  vecx<vec4_> zxyy;
  vecx<vec4_> zxyz;
  vecx<vec4_> zxzw;
  vecx<vec4_> zxzx;
  vecx<vec4_> zxzy;
  vecx<vec4_> zxzz;
  vecx<vec4_> zyww;
  vecx<vec4_> zywx;
  vecx<vec4_> zywy;
  vecx<vec4_> zywz;
  vecx<vec4_> zyxw;
  vecx<vec4_> zyxx;
  vecx<vec4_> zyxy;
  vecx<vec4_> zyxz;
  vecx<vec4_> zyyw;
  vecx<vec4_> zyyx;
  vecx<vec4_> zyyy;
  vecx<vec4_> zyyz;
  vecx<vec4_> zyzw;
  vecx<vec4_> zyzx;
  vecx<vec4_> zyzy;
  vecx<vec4_> zyzz;
  vecx<vec4_> zzww;
  vecx<vec4_> zzwx;
  vecx<vec4_> zzwy;
  vecx<vec4_> zzwz;
  vecx<vec4_> zzxw;
  vecx<vec4_> zzxx;
  vecx<vec4_> zzxy;
  vecx<vec4_> zzxz;
  vecx<vec4_> zzyw;
  vecx<vec4_> zzyx;
  vecx<vec4_> zzyy;
  vecx<vec4_> zzyz;
  vecx<vec4_> zzzw;
  vecx<vec4_> zzzx;
  vecx<vec4_> zzzy;
  vecx<vec4_> zzzz;

  vec4_ operator++(int);
  vec4_ operator--(int);
  vec4_ operator-() const;
  vec4_ operator+(vec4_) const;
  vec4_ operator-(vec4_) const;
  vec4_ operator*(vec4_) const;
  // template <class vec> vec4 operator*(matx<vec>);
  vec4_ operator/(vec4_) const;
  vec4_ operator>>(vec4_) const;
  vec4_ operator<<(vec4_) const;
  vec4_ operator|(vec4_) const;
  vec4_ operator&(vec4_) const;
  vec4_ operator^(vec4_) const;
  bool operator<(vec4_) const;
  bool operator>(vec4_) const;
  bool operator<=(vec4_) const;
  bool operator>=(vec4_) const;
  bool operator==(vec4_) const;
  bool operator!=(vec4_) const;
  bool operator&&(vec4_) const;
  bool operator^(vec4_) const;
  bool operator||(vec4_) const;
  vec4_ operator=(vec4_);
  vec4_ operator+=(vec4_);
  vec4_ operator-=(vec4_);
  vec4_ operator*=(vec4_);
  vec4_ operator/=(vec4_);
  vec4_ operator>>=(vec4_);
  vec4_ operator<<=(vec4_);
  vec4_ operator|=(vec4_);
  vec4_ operator&=(vec4_);
  vec4_ operator^=(vec4_);

  vec4_ operator+(S) const;
  vec4_ operator-(S) const;
  vec4_ operator*(S) const;
  vec4_ operator/(S) const;
  vec4_ operator>>(S) const;
  vec4_ operator<<(S) const;
  vec4_ operator|(S) const;
  vec4_ operator&(S) const;
  vec4_ operator^(S) const;
  vec4_ operator+=(S);
  vec4_ operator-=(S);
  vec4_ operator*=(S);
  vec4_ operator>>=(S);
  vec4_ operator<<=(S);
  vec4_ operator|=(S);
  vec4_ operator&=(S);
  vec4_ operator^=(S);
  vec4_ operator/=(S);
  vec4_ operator=(S);

  vec4_ operator+(S::base_type) const;
  vec4_ operator-(S::base_type) const;
  vec4_ operator*(S::base_type) const;
  vec4_ operator/(S::base_type) const;
  vec4_ operator>>(S::base_type) const;
  vec4_ operator<<(S::base_type) const;
  vec4_ operator|(S::base_type) const;
  vec4_ operator&(S::base_type) const;
  vec4_ operator^(S::base_type) const;
  vec4_ operator+=(S::base_type);
  vec4_ operator-=(S::base_type);
  vec4_ operator*=(S::base_type);
  vec4_ operator/=(S::base_type);
  vec4_ operator>>=(S::base_type);
  vec4_ operator<<=(S::base_type);
  vec4_ operator|=(S::base_type);
  vec4_ operator&=(S::base_type);
  vec4_ operator^=(S::base_type);
};

template <class vec, int n> struct matx {
  using scalar_type = typename vec::scalar_type;
  vec data[n];
  matx() {}
  matx(scalar_type) {}
  vec &operator[](int x) { return data[x]; }
  matx operator++(int);
  matx operator--(int);
  matx operator-() const;

  friend matx operator*(scalar_type, matx);
  friend matx operator/(scalar_type, matx);

  friend vec operator*(matx, vec);

  matx operator+(matx) const;
  matx operator-(matx) const;
  template <class vec_, int n_> matx operator*(matx<vec_, n_>) const;
  template <class vec_> matx operator*(vec_) const;
  matx operator/(scalar_type) const;
  matx operator==(matx) const;
  matx operator!=(matx) const;
  matx operator=(matx);
  matx operator+=(matx);
  matx operator-=(matx);
  matx operator*=(scalar_type);
  matx operator/=(scalar_type);
};

using vec2 = vec2_<float_>;
using vec3 = vec3_<float_>;
using vec4 = vec4_<float_>;
using dvec2 = vec2_<double_>;
using dvec3 = vec3_<double_>;
using dvec4 = vec4_<double_>;
using ivec2 = vec2_<int_>;
using ivec3 = vec3_<int_>;
using ivec4 = vec4_<int_>;
using uvec2 = vec2_<uint>;
using uvec3 = vec3_<uint>;
using uvec4 = vec4_<uint>;

using mat2 = matx<vec2, 2>;
using mat3 = matx<vec3, 3>;
using mat4 = matx<vec4, 4>;
using mat2x2 = matx<vec2, 2>;
using mat3x2 = matx<vec3, 2>;
using mat4x2 = matx<vec4, 2>;
using mat2x3 = matx<vec2, 3>;
using mat3x3 = matx<vec3, 3>;
using mat4x3 = matx<vec4, 3>;
using mat2x4 = matx<vec2, 4>;
using mat3x4 = matx<vec3, 4>;
using mat4x4 = matx<vec4, 4>;

using dmat2 = matx<dvec2, 2>;
using dmat3 = matx<dvec3, 3>;
using dmat4 = matx<dvec4, 4>;
using dmat2x2 = matx<dvec2, 2>;
using dmat3x2 = matx<dvec3, 2>;
using dmat4x2 = matx<dvec4, 2>;
using dmat2x3 = matx<dvec2, 3>;
using dmat3x3 = matx<dvec3, 3>;
using dmat4x3 = matx<dvec4, 3>;
using dmat2x4 = matx<dvec2, 4>;
using dmat3x4 = matx<dvec3, 4>;
using dmat4x4 = matx<dvec4, 4>;

struct sampler1D {};
struct sampler2D {};
struct sampler3D {};

struct sampler2DRect {};

struct sampler1DArray {};
struct sampler2DArray {};

struct samplerBuffer {};

struct sampler2DMS {};
struct sampler2DMSArray {};
struct sample {};

struct samplerCube {};
struct sampler1DShadow {};
struct sampler2DShadow {};
struct samplerCubeShadow {};
struct samplerCubeArray {};
struct sampler1DArrayShadow {};
struct sampler2DArrayShadow {};
struct sampler2DRectShadow {};
struct samplerCubeArrayShadow {};

struct bvec2 {};
struct bvec3 {};
struct bvec4 {};

#define int int_
#define float float_
#define double double_

#if __glslver__ >= 130
float acosh(float);
vec2 acosh(vec2);
vec3 acosh(vec3);
vec4 acosh(vec4);
float asinh(float);
vec2 asinh(vec2);
vec3 asinh(vec3);
vec4 asinh(vec4);
float cosh(float);
vec2 cosh(vec2);
vec3 cosh(vec3);
vec4 cosh(vec4);
float sinh(float);
vec2 sinh(vec2);
vec3 sinh(vec3);
vec4 sinh(vec4);
float tanh(float);
vec2 tanh(vec2);
vec3 tanh(vec3);
vec4 tanh(vec4);
int abs(int);
ivec2 abs(ivec2);
ivec3 abs(ivec3);
ivec4 abs(ivec4);
int clamp(int x, int min, int max);
uint clamp(uint x, uint min, uint max);
ivec2 clamp(ivec2 x, ivec2 min, ivec2 max);
uvec2 clamp(uvec2 x, uvec2 min, uvec2 max);
ivec3 clamp(ivec3 x, ivec3 min, ivec3 max);
uvec3 clamp(uvec3 x, uvec3 min, uvec3 max);
ivec4 clamp(ivec4 x, ivec4 min, ivec4 max);
uvec4 clamp(uvec4 x, uvec4 min, uvec4 max);
ivec2 clamp(ivec2 x, ivec2 min, int max);
uvec2 clamp(uvec2 x, uvec2 min, uint max);
ivec3 clamp(ivec3 x, ivec3 min, int max);
uvec3 clamp(uvec3 x, uvec3 min, uint max);
ivec4 clamp(ivec4 x, ivec4 min, int max);
uvec4 clamp(uvec4 x, uvec4 min, uint max);
float inversesqrt(float);
vec2 inversesqrt(vec2);
vec3 inversesqrt(vec3);
vec4 inversesqrt(vec4);
bool isnan(float);
bvec2 isnan(vec2);
bvec3 isnan(vec3);
bvec4 isnan(vec4);
int min(int, int);
uint min(uint, uint);
ivec2 min(ivec2, ivec2);
uvec2 min(uvec2, uvec2);
ivec3 min(ivec3, ivec3);
uvec3 min(uvec3, uvec3);
ivec4 min(ivec4, ivec4);
uvec4 min(uvec4, uvec4);
ivec2 min(ivec2, ivec2);
uvec2 min(uvec2, uvec2);
ivec3 min(ivec3, ivec3);
uvec3 min(uvec3, uvec3);
ivec4 min(ivec4, ivec4);
uvec4 min(uvec4, uvec4);
int max(int, int);
uint max(uint, uint);
ivec2 max(ivec2, ivec2);
uvec2 max(uvec2, uvec2);
ivec3 max(ivec3, ivec3);
uvec3 max(uvec3, uvec3);
ivec4 max(ivec4, ivec4);
uvec4 max(uvec4, uvec4);
ivec2 max(ivec2, ivec2);
uvec2 max(uvec2, uvec2);
ivec3 max(ivec3, ivec3);
uvec3 max(uvec3, uvec3);
ivec4 max(ivec4, ivec4);
uvec4 max(uvec4, uvec4);
float modf(float x, float i);
vec2 modf(vec2 x, vec2 i);
vec3 modf(vec3 x, vec3 i);
vec4 modf(vec4 x, vec4 i);
float round(float);
vec2 round(vec2);
vec3 round(vec3);
vec4 round(vec4);
float roundEven(float);
vec2 roundEven(vec2);
vec3 roundEven(vec3);
vec4 roundEven(vec4);
float sign(float);
vec2 sign(vec2);
vec3 sign(vec3);
vec4 sign(vec4);
float smoothstep(float edge0, float edge1, float x);
vec2 smoothstep(vec2 edge0, vec2 edge1, vec2 x);
vec3 smoothstep(vec3 edge0, vec3 edge1, vec3 x);
vec4 smoothstep(vec4 edge0, vec4 edge1, vec4 x);
vec2 smoothstep(float edge0, float edge1, vec2 x);
vec3 smoothstep(float edge0, float edge1, vec3 x);
vec4 smoothstep(float edge0, float edge1, vec4 x);
float trunc(float);
vec2 trunc(vec2);
vec3 trunc(vec3);
vec4 trunc(vec4);
bvec2 notEqual(ivec2, ivec2);
bvec2 notEqual(uvec2, uvec2);
bvec3 notEqual(ivec3, ivec3);
bvec3 notEqual(uvec3, uvec3);
bvec4 notEqual(ivec4, ivec4);
bvec4 notEqual(uvec4, uvec4);

#endif

#if __glslver__ >= 150
int sign(int);
ivec2 sign(ivec2);
ivec3 sign(ivec3);
ivec4 sign(ivec4);

#endif

#if __glslver__ >= 400
double ceil(double);
dvec2 ceil(dvec2);
dvec3 ceil(dvec3);
dvec4 ceil(dvec4);
double clamp(double x, double min, double max);
dvec2 clamp(dvec2 x, dvec2 min, dvec2 max);
dvec3 clamp(dvec3 x, dvec3 min, dvec3 max);
dvec4 clamp(dvec4 x, dvec4 min, dvec4 max);
dvec2 clamp(dvec2 x, dvec2 min, double max);
dvec3 clamp(dvec3 x, dvec3 min, double max);
dvec4 clamp(dvec4 x, dvec4 min, double max);
double floor(double);
dvec2 floor(dvec2);
dvec3 floor(dvec3);
dvec4 floor(dvec4);
float fma(float a, float b, float c);
double fma(double a, double b, double c);
vec2 fma(vec2 a, vec2 b, vec2 c);
dvec2 fma(dvec2 a, dvec2 b, dvec2 c);
vec3 fma(vec3 a, vec3 b, vec3 c);
dvec3 fma(dvec3 a, dvec3 b, dvec3 c);
vec4 fma(vec4 a, vec4 b, vec4 c);
dvec4 fma(dvec4 a, dvec4 b, dvec4 c);
double fract(double);
dvec2 fract(dvec2);
dvec3 fract(dvec3);
dvec4 fract(dvec4);
double inversesqrt(double);
dvec2 inversesqrt(dvec2);
dvec3 inversesqrt(dvec3);
dvec4 inversesqrt(dvec4);
bool isinf(double);
bvec2 isinf(dvec2);
bvec3 isinf(dvec3);
bvec4 isinf(dvec4);
bool isnan(double);
bvec2 isnan(dvec2);
bvec3 isnan(dvec3);
bvec4 isnan(dvec4);
double min(double, double);
dvec2 min(dvec2, dvec2);
dvec3 min(dvec3, dvec3);
dvec4 min(dvec4, dvec4);
dvec2 min(dvec2, dvec2);
dvec3 min(dvec3, dvec3);
dvec4 min(dvec4, dvec4);
double max(double, double);
dvec2 max(dvec2, dvec2);
dvec3 max(dvec3, dvec3);
dvec4 max(dvec4, dvec4);
dvec2 max(dvec2, dvec2);
dvec3 max(dvec3, dvec3);
dvec4 max(dvec4, dvec4);
double mix(double x, double y, double a);
dvec2 mix(dvec2 x, dvec2 y, dvec2 a);
dvec3 mix(dvec3 x, dvec3 y, dvec3 a);
dvec4 mix(dvec4 x, dvec4 y, dvec4 a);
dvec2 mix(dvec2 x, dvec2 y, double a);
dvec3 mix(dvec3 x, dvec3 y, double a);
dvec4 mix(dvec4 x, dvec4 y, double a);
dvec2 mix(dvec2 x, dvec2 y, bool a);
dvec2 mix(dvec2 x, dvec2 y, bvec2 a);
dvec3 mix(dvec3 x, dvec3 y, bvec3 a);
dvec4 mix(dvec4 x, dvec4 y, bvec4 a);
double mod(double x, double y);
dvec2 mod(dvec2 x, dvec2 y);
dvec3 mod(dvec3 x, dvec3 y);
dvec4 mod(dvec4 x, dvec4 y);
dvec2 mod(dvec2 x, dvec2 y);
dvec3 mod(dvec3 x, dvec3 y);
dvec4 mod(dvec4 x, dvec4 y);
double modf(double x, double i);
dvec2 modf(dvec2 x, dvec2 i);
dvec3 modf(dvec3 x, dvec3 i);
dvec4 modf(dvec4 x, dvec4 i);
double round(double);
dvec2 round(dvec2);
dvec3 round(dvec3);
dvec4 round(dvec4);
double roundEven(double);
dvec2 roundEven(dvec2);
dvec3 roundEven(dvec3);
dvec4 roundEven(dvec4);
double sign(double);
dvec2 sign(dvec2);
dvec3 sign(dvec3);
dvec4 sign(dvec4);
double smoothstep(double edge0, double edge1, double x);
dvec2 smoothstep(dvec2 edge0, dvec2 edge1, dvec2 x);
dvec3 smoothstep(dvec3 edge0, dvec3 edge1, dvec3 x);
dvec4 smoothstep(dvec4 edge0, dvec4 edge1, dvec4 x);
dvec2 smoothstep(double edge0, double edge1, dvec2 x);
dvec3 smoothstep(double edge0, double edge1, dvec3 x);
dvec4 smoothstep(double edge0, double edge1, dvec4 x);
double sqrt(double);
dvec2 sqrt(dvec2);
dvec3 sqrt(dvec3);
dvec4 sqrt(dvec4);
double step(double edge, double x);
dvec2 step(dvec2 edge, dvec2 x);
dvec3 step(dvec3 edge, dvec3 x);
dvec4 step(dvec4 edge, dvec4 x);
dvec2 step(double edge, dvec2 x);
dvec3 step(double edge, dvec3 x);
dvec4 step(double edge, dvec4 x);
double trunc(double);
dvec2 trunc(dvec2);
dvec3 trunc(dvec3);
dvec4 trunc(dvec4);
dvec3 cross(dvec3, dvec3);
double distance(double, double);
double distance(dvec2, dvec2);
double distance(dvec3, dvec3);
double distance(dvec4, dvec4);
double dot(double, double);
double dot(dvec2, dvec2);
double dot(dvec3, dvec3);
double dot(dvec4, dvec4);
double faceforward(double N, double I, double Nref);
dvec2 faceforward(dvec2 N, dvec2 I, dvec2 Nref);
dvec3 faceforward(dvec3 N, dvec3 I, dvec3 Nref);
dvec4 faceforward(dvec4 N, dvec4 I, dvec4 Nref);
double length(double);
double length(dvec2);
double length(dvec3);
double length(dvec4);
double normalize(double);
dvec2 normalize(dvec2);
dvec3 normalize(dvec3);
dvec4 normalize(dvec4);
double reflect(double I, double N);
dvec2 reflect(dvec2 I, dvec2 N);
dvec3 reflect(dvec3 I, dvec3 N);
dvec4 reflect(dvec4 I, dvec4 N);
double refract(double I, double N, float eta);
dvec2 refract(dvec2 I, dvec2 N, float eta);
dvec3 refract(dvec3 I, dvec3 N, float eta);
dvec4 refract(dvec4 I, dvec4 N, float eta);

#endif

#if __glslver__ >= 410
double abs(double);
dvec2 abs(dvec2);
dvec3 abs(dvec3);
dvec4 abs(dvec4);
#endif

#if __glslver__ >= 450
float dFdxCoarse(float);
vec2 dFdxCoarse(vec2);
vec3 dFdxCoarse(vec3);
vec4 dFdxCoarse(vec4);
float dFdxFine(float);
vec2 dFdxFine(vec2);
vec3 dFdxFine(vec3);
vec4 dFdxFine(vec4);
float dFdyCoarse(float);
vec2 dFdyCoarse(vec2);
vec3 dFdyCoarse(vec3);
vec4 dFdyCoarse(vec4);
float dFdyFine(float);
vec2 dFdyFine(vec2);
vec3 dFdyFine(vec3);
vec4 dFdyFine(vec4);
float fwidthCoarse(float);
vec2 fwidthCoarse(vec2);
vec3 fwidthCoarse(vec3);
vec4 fwidthCoarse(vec4);
float fwidthFine(float);
vec2 fwidthFine(vec2);
vec3 fwidthFine(vec3);
vec4 fwidthFine(vec4);
int mix(int x, int y, bool a);
uint mix(uint x, uint y, bool a);
ivec2 mix(ivec2 x, ivec2 y, bvec2 a);
uvec2 mix(uvec2 x, uvec2 y, bvec2 a);
ivec3 mix(ivec3 x, ivec3 y, bvec3 a);
uvec3 mix(uvec3 x, uvec3 y, bvec3 a);
ivec4 mix(ivec4 x, ivec4 y, bvec4 a);
uvec4 mix(uvec4 x, uvec4 y, bvec4 a);
ivec2 mix(ivec2 x, ivec2 y, bool a);
uvec2 mix(uvec2 x, uvec2 y, bool a);
ivec3 mix(ivec3 x, ivec3 y, bool a);
uvec3 mix(uvec3 x, uvec3 y, bool a);
ivec4 mix(ivec4 x, ivec4 y, bool a);
uvec4 mix(uvec4 x, uvec4 y, bool a);
bool mix(bool x, bool y, bool a);
bvec2 mix(bvec2 x, bvec2 y, bvec2 a);
bvec3 mix(bvec3 x, bvec3 y, bvec3 a);
bvec4 mix(bvec4 x, bvec4 y, bvec4 a);
#endif

float acos(float);
float asin(float);
float atan(float, float);
float atan(float);
float atanh(float);
float cos(float);
float degrees(float);
float radians(float);
float sin(float);
float tan(float);

vec2 acos(vec2);
vec2 asin(vec2);
vec2 atan(vec2, vec2);
vec2 atan(vec2);
vec2 atanh(vec2);
vec2 cos(vec2);
vec2 degrees(vec2);
vec2 radians(vec2);
vec2 sin(vec2);

vec2 tan(vec2);
vec3 acos(vec3);
vec3 asin(vec3);
vec3 atan(vec3, vec3);
vec3 atan(vec3);
vec3 atanh(vec3);
vec3 cos(vec3);
vec3 degrees(vec3);
vec3 radians(vec3);
vec3 sin(vec3);
vec3 tan(vec3);

vec4 acos(vec4);
vec4 asin(vec4);
vec4 atan(vec4, vec4);
vec4 atan(vec4);
vec4 atanh(vec4);
vec4 cos(vec4);
vec4 degrees(vec4);
vec4 radians(vec4);
vec4 sin(vec4);
vec4 tan(vec4);

float abs(float);
vec2 abs(vec2);
vec3 abs(vec3);
vec4 abs(vec4);

float ceil(float);
vec2 ceil(vec2);
vec3 ceil(vec3);
vec4 ceil(vec4);

float clamp(float x, float min, float max);
vec2 clamp(vec2 x, vec2 min, vec2 max);
vec3 clamp(vec3 x, vec3 min, vec3 max);
vec4 clamp(vec4 x, vec4 min, vec4 max);
vec2 clamp(vec2 x, vec2 min, float max);
vec3 clamp(vec3 x, vec3 min, float max);
vec4 clamp(vec4 x, vec4 min, float max);

float dFdx(float);
vec2 dFdx(vec2);
vec3 dFdx(vec3);
vec4 dFdx(vec4);

float dFdy(float);
vec2 dFdy(vec2);
vec3 dFdy(vec3);
vec4 dFdy(vec4);

float exp(float);
vec2 exp(vec2);
vec3 exp(vec3);
vec4 exp(vec4);

float exp2(float);
vec2 exp2(vec2);
vec3 exp2(vec3);
vec4 exp2(vec4);

float floor(float);
vec2 floor(vec2);
vec3 floor(vec3);
vec4 floor(vec4);

float fract(float);
vec2 fract(vec2);
vec3 fract(vec3);
vec4 fract(vec4);

float fwidth(float);
vec2 fwidth(vec2);
vec3 fwidth(vec3);
vec4 fwidth(vec4);

bool isinf(float);
bvec2 isinf(vec2);
bvec3 isinf(vec3);
bvec4 isinf(vec4);

float log(float);
vec2 log(vec2);
vec3 log(vec3);
vec4 log(vec4);

float log2(float);
vec2 log2(vec2);
vec3 log2(vec3);
vec4 log2(vec4);

float min(float, float);
vec2 min(vec2, vec2);
vec3 min(vec3, vec3);
vec4 min(vec4, vec4);
vec2 min(vec2, vec2);
vec3 min(vec3, vec3);
vec4 min(vec4, vec4);

float max(float, float);
vec2 max(vec2, vec2);
vec3 max(vec3, vec3);
vec4 max(vec4, vec4);
vec2 max(vec2, vec2);
vec3 max(vec3, vec3);
vec4 max(vec4, vec4);

float mix(float x, float y, float a);
vec2 mix(vec2 x, vec2 y, vec2 a);
vec3 mix(vec3 x, vec3 y, vec3 a);
vec4 mix(vec4 x, vec4 y, vec4 a);
vec2 mix(vec2 x, vec2 y, float a);
vec3 mix(vec3 x, vec3 y, float a);
vec4 mix(vec4 x, vec4 y, float a);
vec2 mix(vec2 x, vec2 y, bool a);
vec2 mix(vec2 x, vec2 y, bvec2 a);
vec3 mix(vec3 x, vec3 y, bvec3 a);
vec4 mix(vec4 x, vec4 y, bvec4 a);

float mod(float x, float y);
vec2 mod(vec2 x, vec2 y);
vec3 mod(vec3 x, vec3 y);
vec4 mod(vec4 x, vec4 y);
vec2 mod(vec2 x, vec2 y);
vec3 mod(vec3 x, vec3 y);
vec4 mod(vec4 x, vec4 y);

float noise(float);
float noise(vec2);
float noise(vec3);
float noise(vec4);

vec2 noise2(float);
vec2 noise2(vec2);
vec2 noise2(vec3);
vec2 noise2(vec4);

vec3 noise3(float);
vec3 noise3(vec2);
vec3 noise3(vec3);
vec3 noise3(vec4);

vec4 noise4(float);
vec4 noise4(vec2);
vec4 noise4(vec3);
vec4 noise4(vec4);

float pow(float, float);
vec2 pow(vec2, vec2);
vec3 pow(vec3, vec3);
vec4 pow(vec4, vec4);

float sqrt(float);
vec2 sqrt(vec2);
vec3 sqrt(vec3);
vec4 sqrt(vec4);

float step(float edge, float x);
vec2 step(vec2 edge, vec2 x);
vec3 step(vec3 edge, vec3 x);
vec4 step(vec4 edge, vec4 x);
vec2 step(float edge, vec2 x);
vec3 step(float edge, vec3 x);
vec4 step(float edge, vec4 x);

vec3 cross(vec3, vec3);

float distance(float, float);
float distance(vec2, vec2);
float distance(vec3, vec3);
float distance(vec4, vec4);

float dot(float, float);
float dot(vec2, vec2);
float dot(vec3, vec3);
float dot(vec4, vec4);

float faceforward(float N, float I, float Nref);
vec2 faceforward(vec2 N, vec2 I, vec2 Nref);
vec3 faceforward(vec3 N, vec3 I, vec3 Nref);
vec4 faceforward(vec4 N, vec4 I, vec4 Nref);

float length(float);
float length(vec2);
float length(vec3);
float length(vec4);

float normalize(float);
vec2 normalize(vec2);
vec3 normalize(vec3);
vec4 normalize(vec4);

float reflect(float I, float N);
vec2 reflect(vec2 I, vec2 N);
vec3 reflect(vec3 I, vec3 N);
vec4 reflect(vec4 I, vec4 N);

float refract(float I, float N, float eta);
vec2 refract(vec2 I, vec2 N, float eta);
vec3 refract(vec3 I, vec3 N, float eta);
vec4 refract(vec4 I, vec4 N, float eta);

bvec2 equal(vec2, vec2);
bvec2 equal(ivec2, ivec2);
bvec2 equal(uvec2, uvec2);

bvec3 equal(vec3, vec3);
bvec3 equal(ivec3, ivec3);
bvec3 equal(uvec3, uvec3);

bvec4 equal(vec4, vec4);
bvec4 equal(ivec4, ivec4);
bvec4 equal(uvec4, uvec4);

bvec2 notEqual(vec2, vec2);
bvec3 notEqual(vec3, vec3);
bvec4 notEqual(vec4, vec4);

#if __glslver__ >= 330
int floatBitsToInt(float);
ivec2 floatBitsToInt(vec2);
ivec3 floatBitsToInt(vec3);
ivec4 floatBitsToInt(vec4);

uint floatBitsToUint(float);
uvec2 floatBitsToUint(vec2);
uvec3 floatBitsToUint(vec3);
uvec4 floatBitsToUint(vec4);

float intBitsToFloat(int);
float intBitsToFloat(ivec2);
float intBitsToFloat(ivec3);
float intBitsToFloat(ivec4);

float uintBitsToFloat(uint);
float uintBitsToFloat(ivec2);
float uintBitsToFloat(ivec3);
float uintBitsToFloat(ivec4);

#endif

#if __glslver__ >= 400
float frexp(float x, int exp);
vec2 frexp(vec2 x, ivec2 exp);
vec3 frexp(vec3 x, ivec3 exp);
vec4 frexp(vec4 x, ivec4 exp);

double frexp(double x, int exp);
dvec2 frexp(dvec2 x, ivec2 exp);
dvec3 frexp(dvec3 x, ivec3 exp);
dvec4 frexp(dvec4 x, ivec4 exp);

float ldexp(float x, int exp);
vec2 ldexp(vec2 x, ivec2 exp);
vec3 ldexp(vec3 x, ivec3 exp);
vec4 ldexp(vec4 x, ivec4 exp);
double ldexp(double x, int exp);
dvec2 ldexp(dvec2 x, ivec2 exp);
dvec3 ldexp(dvec3 x, ivec3 exp);
dvec4 ldexp(dvec4 x, ivec4 exp);

double packDouble2x32(uvec2 v);
uvec2 unpackDouble2x32(double d);
#endif

#if __glslver__ >= 410
uint packUnorm2x16(vec2 v);
uint packUnorm4x8(vec4 v);
uint packSnorm4x8(vec4 v);
vec2 unpackUnorm2x16(uint p);
vec4 unpackUnorm4x8(uint p);
vec4 unpackSnorm4x8(uint p);
#endif

#if __glslver__ >= 420
uint packHalf2x16(vec2 v);
vec2 unpackHalf2x16(uint v);
uint packSnorm2x16(vec2 v);
vec2 unpackSnorm2x16(uint p);
#endif

bool all(bvec2);
bool all(bvec3);
bool all(bvec4);

bool any(bvec2);
bool any(bvec3);
bool any(bvec4);

bvec2 not_(bvec2);
bvec3 not_(bvec3);
bvec4 not_(bvec4);

// #define not not_

bvec2 greaterThan(vec2, vec2);
bvec3 greaterThan(vec3, vec3);
bvec4 greaterThan(vec4, vec4);
bvec2 greaterThan(ivec2, ivec2);
bvec3 greaterThan(ivec3, ivec3);
bvec4 greaterThan(ivec4, ivec4);

bvec2 greaterThanEqual(vec2, vec2);
bvec3 greaterThanEqual(vec3, vec3);
bvec4 greaterThanEqual(vec4, vec4);
bvec2 greaterThanEqual(ivec2, ivec2);
bvec3 greaterThanEqual(ivec3, ivec3);
bvec4 greaterThanEqual(ivec4, ivec4);

bvec2 lessThan(vec2, vec2);
bvec3 lessThan(vec3, vec3);
bvec4 lessThan(vec4, vec4);
bvec2 lessThan(ivec2, ivec2);
bvec3 lessThan(ivec3, ivec3);
bvec4 lessThan(ivec4, ivec4);

bvec2 lessThanEqual(vec2, vec2);
bvec3 lessThanEqual(vec3, vec3);
bvec4 lessThanEqual(vec4, vec4);
bvec2 lessThanEqual(ivec2, ivec2);
bvec3 lessThanEqual(ivec3, ivec3);
bvec4 lessThanEqual(ivec4, ivec4);

#if __glslver__ >= 130
bvec2 greaterThan(uvec2, uvec2);
bvec3 greaterThan(uvec3, uvec3);
bvec4 greaterThan(uvec4, uvec4);

bvec2 greaterThanEqual(uvec2, uvec2);
bvec3 greaterThanEqual(uvec3, uvec3);
bvec4 greaterThanEqual(uvec4, uvec4);

bvec2 lessThan(uvec2, uvec2);
bvec3 lessThan(uvec3, uvec3);
bvec4 lessThan(uvec4, uvec4);

bvec2 lessThanEqual(uvec2, uvec2);
bvec3 lessThanEqual(uvec3, uvec3);
bvec4 lessThanEqual(uvec4, uvec4);
#endif

#if __glslver__ >= 130
vec4 texelFetch(sampler1D sampler, int P, int lod);
vec4 texelFetch(sampler2D sampler, ivec2 P, int lod);
vec4 texelFetch(sampler3D sampler, ivec3 P, int lod);
vec4 texelFetch(sampler1DArray sampler, ivec2 P, int lod);
vec4 texelFetch(sampler2DArray sampler, ivec3 P, int lod);
vec4 texelFetchOffset(sampler1D sampler, int P, int lod, int offset);
vec4 texelFetchOffset(sampler2D sampler, ivec2 P, int lod, int offset);
vec4 texelFetchOffset(sampler3D sampler, ivec3 P, int lod, int offset);
vec4 texelFetchOffset(sampler1DArray sampler, ivec2 P, int lod, int offset);
vec4 texelFetchOffset(sampler2DArray sampler, ivec3 P, int lod, int offset);
vec4 texture(sampler1D sampler, float P, float bias = 0);
vec4 texture(sampler2D sampler, vec2 P, float bias = 0);
vec4 texture(sampler3D sampler, vec3 P, float bias = 0);
vec4 texture(samplerCube sampler, vec3 P, float bias = 0);
float texture(sampler1DShadow sampler, vec3 P, float bias = 0);
float texture(sampler2DShadow sampler, vec3 P, float bias = 0);
float texture(samplerCubeShadow sampler, vec3 P, float bias = 0);
vec4 texture(sampler1DArray sampler, vec2 P, float bias = 0);
vec4 texture(sampler2DArray sampler, vec3 P, float bias = 0);
float texture(sampler1DArrayShadow sampler, vec3 P, float bias = 0);
float texture(sampler2DArrayShadow sampler, vec4 P, float bias = 0);
vec4 textureGrad(sampler1D sampler, float P, float dPdx, float dPdy);
vec4 textureGrad(sampler2D sampler, vec2 P, vec2 dPdx, vec2 dPdy);
vec4 textureGrad(sampler3D sampler, vec3 P, vec3 dPdx, vec3 dPdy);
vec4 textureGrad(samplerCube sampler, vec3 P, vec3 dPdx, vec3 dPdy);
float textureGrad(sampler1DShadow sampler, vec3 P, float dPdx, float dPdy);
float textureGrad(sampler2DShadow sampler, vec3 P, vec2 dPdx, vec2 dPdy);
vec4 textureGrad(sampler1DArray sampler, vec2 P, float dPdx, float dPdy);
vec4 textureGrad(sampler2DArray sampler, vec3 P, vec2 dPdx, vec2 dPdy);
float textureGrad(sampler1DArrayShadow sampler, vec3 P, float dPdx, float dPdy);
vec4 textureGradOffset(sampler1D sampler, float P, float dPdx, float dPdy,
                       int offset);
vec4 textureGradOffset(sampler2D sampler, vec2 P, vec2 dPdx, vec2 dPdy,
                       ivec2 offset);
vec4 textureGradOffset(sampler3D sampler, vec3 P, vec3 dPdx, vec3 dPdy,
                       ivec3 offset);
float textureGradOffset(sampler1DShadow sampler, vec3 P, float dPdx, float dPdy,
                        int offset);
float textureGradOffset(sampler2DShadow sampler, vec3 P, vec2 dPdx, vec2 dPdy,
                        ivec2 offset);
vec4 textureGradOffset(sampler1DArray sampler, vec2 P, float dPdx, float dPdy,
                       int offset);
vec4 textureGradOffset(sampler2DArray sampler, vec3 P, vec2 dPdx, vec2 dPdy,
                       ivec2 offset);
float textureGradOffset(sampler1DArrayShadow sampler, vec3 P, float dPdx,
                        float dPdy, int offset);
float textureGradOffset(sampler2DArrayShadow sampler, vec3 P, vec2 dPdx,
                        vec2 dPdy, ivec2 offset);
vec4 textureLod(sampler1D sampler, float P, float lod);
vec4 textureLod(sampler2D sampler, vec2 P, float lod);
vec4 textureLod(sampler3D sampler, vec3 P, float lod);
vec4 textureLod(samplerCube sampler, vec3 P, float lod);
float textureLod(sampler1DShadow sampler, vec3 P, float lod);
float textureLod(sampler2DShadow sampler, vec4 P, float lod);
vec4 textureLod(sampler1DArray sampler, vec2 P, float lod);
vec4 textureLod(sampler2DArray sampler, vec3 P, float lod);
float textureLod(sampler1DArrayShadow sampler, vec3 P, float lod);
vec4 textureLodOffset(sampler1D sampler, float P, float lod, int offset);
vec4 textureLodOffset(sampler2D sampler, vec2 P, float lod, ivec2 offset);
vec4 textureLodOffset(sampler3D sampler, vec3 P, float lod, ivec3 offset);
float textureLodOffset(sampler1DShadow sampler, vec3 P, float lod, int offset);
float textureLodOffset(sampler2DShadow sampler, vec4 P, float lod,
                       ivec2 offset);
vec4 textureLodOffset(sampler1DArray sampler, vec2 P, float lod, int offset);
vec4 textureLodOffset(sampler2DArray sampler, vec3 P, float lod, ivec2 offset);
float textureLodOffset(sampler1DArrayShadow sampler, vec3 P, float lod,
                       int offset);
vec4 textureOffset(sampler1D sampler, float P, int offset, float bias = 0);
vec4 textureOffset(sampler2D sampler, vec2 P, ivec2 offset, float bias = 0);
vec4 textureOffset(sampler3D sampler, vec3 P, ivec3 offset, float bias = 0);
float textureOffset(sampler1DShadow sampler, vec3 P, int offset,
                    float bias = 0);
float textureOffset(sampler2DShadow sampler, vec4 P, ivec2 offset,
                    float bias = 0);
vec4 textureOffset(sampler1DArray sampler, vec2 P, int offset, float bias = 0);
vec4 textureOffset(sampler2DArray sampler, vec3 P, ivec2 offset,
                   float bias = 0);
float textureOffset(sampler1DArrayShadow sampler, vec3 P, int offset);
float textureOffset(sampler2DArrayShadow sampler, vec4 P, vec2 offset);
vec4 textureProj(sampler1D sampler, vec2 P, float bias = 0);
vec4 textureProj(sampler1D sampler, vec4 P, float bias = 0);
vec4 textureProj(sampler2D sampler, vec3 P, float bias = 0);
vec4 textureProj(sampler2D sampler, vec4 P, float bias = 0);
vec4 textureProj(sampler3D sampler, vec4 P, float bias = 0);
float textureProj(sampler1DShadow sampler, vec4 P, float bias = 0);
float textureProj(sampler2DShadow sampler, vec4 P, float bias = 0);
vec4 textureProjGrad(sampler1D sampler, vec2 P, float pDx, float pDy);
vec4 textureProjGrad(sampler1D sampler, vec4 P, float pDx, float pDy);
vec4 textureProjGrad(sampler2D sampler, vec3 P, vec2 pDx, vec2 pDy);
vec4 textureProjGrad(sampler2D sampler, vec4 P, vec2 pDx, vec2 pDy);
vec4 textureProjGrad(sampler3D sampler, vec4 P, vec3 pDx, vec3 pDy);
float textureProjGrad(sampler1DShadow sampler, vec4 P, float pDx, float pDy);
float textureProjGrad(sampler2DShadow sampler, vec4 P, vec2 pDx, vec2 pDy);
vec4 textureProjGradOffset(sampler1D sampler, vec2 P, float dPdx, float dPdy,
                           int offset);
vec4 textureProjGradOffset(sampler1D sampler, vec4 P, float dPdx, float dPdy,
                           int offset);
vec4 textureProjGradOffset(sampler2D sampler, vec3 P, vec2 dPdx, vec2 dPdy,
                           ivec2 offset);
vec4 textureProjGradOffset(sampler2D sampler, vec4 P, vec2 dPdx, vec2 dPdy,
                           ivec2 offset);
vec4 textureProjGradOffset(sampler3D sampler, vec4 P, vec3 dPdx, vec3 dPdy,
                           ivec3 offset);
float textureProjGradOffset(sampler1DShadow sampler, vec4 P, float dPdx,
                            float dPdy, int offset);
float textureProjGradOffset(sampler2DShadow sampler, vec4 P, vec2 dPdx,
                            vec2 dPdy, ivec2 offset);
vec4 textureProjLod(sampler1D sampler, vec2 P, float lod);
vec4 textureProjLod(sampler1D sampler, vec4 P, float lod);
vec4 textureProjLod(sampler2D sampler, vec3 P, float lod);
vec4 textureProjLod(sampler2D sampler, vec4 P, float lod);
vec4 textureProjLod(sampler3D sampler, vec4 P, float lod);
float textureProjLod(sampler1DShadow sampler, vec4 P, float lod);
float textureProjLod(sampler2DShadow sampler, vec4 P, float lod);
vec4 textureProjLodOffset(sampler1D sampler, vec2 P, float lod, int offset);
vec4 textureProjLodOffset(sampler1D sampler, vec4 P, float lod, int offset);
vec4 textureProjLodOffset(sampler2D sampler, vec3 P, float lod, ivec2 offset);
vec4 textureProjLodOffset(sampler2D sampler, vec4 P, float lod, ivec2 offset);
vec4 textureProjLodOffset(sampler3D sampler, vec4 P, float lod, ivec3 offset);
float textureProjLodOffset(sampler1DShadow sampler, vec4 P, float lod,
                           int offset);
float textureProjLodOffset(sampler2DShadow sampler, vec4 P, float lod,
                           ivec2 offset);
vec4 textureProjOffset(sampler1D sampler, vec2 P, int offset, float bias = 0);
vec4 textureProjOffset(sampler1D sampler, vec4 P, int offset, float bias = 0);
vec4 textureProjOffset(sampler2D sampler, vec3 P, ivec2 offset, float bias = 0);
vec4 textureProjOffset(sampler2D sampler, vec4 P, ivec2 offset, float bias = 0);
vec4 textureProjOffset(sampler3D sampler, vec4 P, ivec3 offset, float bias = 0);
float textureProjOffset(sampler1DShadow sampler, vec4 P, int offset,
                        float bias = 0);
float textureProjOffset(sampler2DShadow sampler, vec4 P, ivec2 offset,
                        float bias = 0);
int textureSize(sampler1D sampler, int lod);
ivec2 textureSize(sampler2D sampler, int lod);
ivec3 textureSize(sampler3D sampler, int lod);
int textureSize(sampler1DShadow sampler, int lod);
ivec2 textureSize(sampler2DShadow sampler, int lod);
ivec2 textureSize(sampler1DArray sampler, int lod);
ivec3 textureSize(sampler2DArray sampler, int lod);
ivec2 textureSize(sampler1DArrayShadow sampler, int lod);
ivec3 textureSize(sampler2DArrayShadow sampler, int lod);
#endif

#if __glslver__ >= 140
vec4 texelFetch(sampler2DRect sampler, ivec2 P);
vec4 texelFetch(samplerBuffer sampler, int P);
vec4 texelFetchOffset(sampler2DRect sampler, ivec2 P, int offset);
vec4 texture(sampler2DRect sampler, vec2 P);
float texture(sampler2DRectShadow sampler, vec3 P);
vec4 textureGrad(sampler2DRect sampler, vec2 P, vec2 dPdx, vec2 dPdy);
float textureGrad(sampler2DRectShadow sampler, vec2 P, vec2 dPdx, vec2 dPdy);
vec4 textureGradOffset(sampler2DRect sampler, vec2 P, vec2 dPdx, vec2 dPdy,
                       ivec2 offset);
float textureGradOffset(sampler2DRectShadow sampler, vec3 P, vec2 dPdx,
                        vec2 dPdy, ivec2 offset);
vec4 textureLod(sampler2DRect sampler, vec2 P, float lod);
float textureLod(sampler2DRectShadow sampler, vec3 P, float lod);
vec4 textureLodOffset(sampler2DRect sampler, vec2 P, float lod, vec2 offset);
float textureLodOffset(sampler2DRectShadow sampler, vec3 P, float lod,
                       vec3 offset);
vec4 textureOffset(sampler2DRect sampler, vec2 P, ivec2 offset);
float textureOffset(sampler2DRectShadow sampler, vec3 P, ivec2 offset);
vec4 textureProj(sampler2DRect sampler, vec3 P);
vec4 textureProj(sampler2DRect sampler, vec4 P);
float textureProj(sampler2DRectShadow sampler, vec4 P);

vec4 textureProjGrad(sampler2DRect sampler, vec3 P, vec2 pDx, vec2 pDy);
vec4 textureProjGrad(sampler2DRect sampler, vec4 P, vec2 pDx, vec2 pDy);
vec4 textureProjGradOffset(sampler2DRect sampler, vec3 P, vec2 dPdx, vec2 dPdy,
                           ivec2 offset);
vec4 textureProjGradOffset(sampler2DRect sampler, vec4 P, vec2 dPdx, vec2 dPdy,
                           ivec2 offset);
vec4 textureProjOffset(sampler2DRect sampler, vec3 P, ivec2 offset);
vec4 textureProjOffset(sampler2DRect sampler, vec4 P, ivec2 offset);
float textureProjGrad(sampler2DRectShadow sampler, vec4 P, vec2 pDx, vec2 pDy);
float textureProjGradOffset(sampler2DRectShadow sampler, vec4 P, vec2 dPdx,
                            vec2 dPdy, ivec2 offset);
float textureProjOffset(sampler2DRectShadow sampler, vec4 P, ivec2 offset);
ivec2 textureSize(sampler2DRect sampler);
ivec2 textureSize(sampler2DRectShadow sampler);
int textureSize(samplerBuffer sampler);
#endif

#if __glslver__ >= 150
vec4 texelFetch(sampler2DMS sampler, ivec2 P, sample sample);
vec4 texelFetch(sampler2DMSArray sampler, ivec3 P, sample sample);
vec4 texelFetchOffset(sampler2DMS sampler, ivec2 P, int lod, int offset,
                      sample sample);
vec4 texelFetchOffset(sampler2DMSArray sampler, ivec3 P, int lod, int offset,
                      sample sample);
ivec2 textureSize(sampler2DMS sampler);
ivec3 textureSize(sampler2DMSArray sampler);
#endif

#if __glslver__ >= 400
float interpolateAtCentroid(float interpolant);
vec2 interpolateAtCentroid(vec2 interpolant);
vec3 interpolateAtCentroid(vec3 interpolant);
vec4 interpolateAtCentroid(vec4 interpolant);
float interpolateAtOffset(float interpolant, vec2 offset);
vec2 interpolateAtOffset(vec2 interpolant, vec2 offset);
vec3 interpolateAtOffset(vec3 interpolant, vec2 offset);
vec4 interpolateAtOffset(vec4 interpolant, vec2 offset);
float interpolateAtSample(float interpolant, int sample);
vec2 interpolateAtSample(vec2 interpolant, int sample);
vec3 interpolateAtSample(vec3 interpolant, int sample);
vec4 interpolateAtSample(vec4 interpolant, int sample);
vec4 texture(samplerCubeArray sampler, vec4 P, float bias = 0);
float texture(samplerCubeArrayShadow sampler, vec4 P, float compare);
vec4 textureGather(sampler2D sampler, vec2 P, int comp = -1);
vec4 textureGather(sampler2DArray sampler, vec3 P, int comp = -1);
vec4 textureGather(samplerCube sampler, vec3 P, int comp = -1);
vec4 textureGather(samplerCubeArray sampler, vec4 P, int comp = -1);
vec4 textureGather(sampler2DRect sampler, vec3 P, int comp = -1);
vec4 textureGather(sampler2DShadow sampler, vec2 P, float refZ);
vec4 textureGather(sampler2DArrayShadow sampler, vec3 P, float refZ);
vec4 textureGather(samplerCubeShadow sampler, vec3 P, float refZ);
vec4 textureGather(samplerCubeArrayShadow sampler, vec4 P, float refZ);
vec4 textureGather(sampler2DRectShadow sampler, vec3 P, float refZ);
vec4 textureGatherOffset(sampler2D sampler, vec2 P, ivec2 offset,
                         int comp = -1);
vec4 textureGatherOffset(sampler2DArray sampler, vec3 P, ivec2 offset,
                         int comp = -1);
vec4 textureGatherOffset(sampler2DRect sampler, vec3 P, ivec2 offset,
                         int comp = -1);
vec4 textureGatherOffset(sampler2DShadow sampler, vec2 P, float refZ,
                         ivec2 offset);
vec4 textureGatherOffset(sampler2DArrayShadow sampler, vec3 P, float refZ,
                         ivec2 offset);
vec4 textureGatherOffset(sampler2DRectShadow sampler, vec3 P, float refZ,
                         ivec2 offset);
vec4 textureGatherOffsets(sampler2D sampler, vec2 P, ivec2 offsets[4],
                          int comp = -1);
vec4 textureGatherOffsets(sampler2DArray sampler, vec3 P, ivec2 offsets[4],
                          int comp = -1);
vec4 textureGatherOffsets(sampler2DRect sampler, vec3 P, ivec2 offsets[4],
                          int comp = -1);
vec4 textureGatherOffsets(sampler2DShadow sampler, vec2 P, float refZ,
                          ivec2 offsets[4]);
vec4 textureGatherOffsets(sampler2DArrayShadow sampler, vec3 P, float refZ,
                          ivec2 offsets[4]);
vec4 textureGatherOffsets(sampler2DRectShadow sampler, vec3 P, float refZ,
                          ivec2 offsets[4]);
vec4 textureGrad(samplerCubeArray sampler, vec4 P, vec3 dPdx, vec3 dPdy);
vec4 textureLod(samplerCubeArray sampler, vec4 P, float lod);
vec4 textureLodOffset(samplerCubeArray sampler, vec4 P, float lod, vec4 offset);
vec2 textureQueryLod(sampler1D sampler, float P);
vec2 textureQueryLod(sampler2D sampler, vec2 P);
vec2 textureQueryLod(sampler3D sampler, vec3 P);
vec2 textureQueryLod(samplerCube sampler, vec3 P);
vec2 textureQueryLod(sampler1DArray sampler, float P);
vec2 textureQueryLod(sampler2DArray sampler, vec2 P);
vec2 textureQueryLod(samplerCubeArray sampler, vec3 P);
vec2 textureQueryLod(sampler1DShadow sampler, float P);
vec2 textureQueryLod(sampler2DShadow sampler, vec2 P);
vec2 textureQueryLod(samplerCubeShadow sampler, vec3 P);
vec2 textureQueryLod(sampler1DArrayShadow sampler, float P);
vec2 textureQueryLod(sampler2DArrayShadow sampler, vec2 P);
vec2 textureQueryLod(samplerCubeArrayShadow sampler, vec3 P);
ivec2 textureSize(samplerCubeShadow sampler, int lod);
ivec2 textureSize(samplerCube sampler, int lod);
ivec3 textureSize(samplerCubeArray sampler, int lod);
ivec3 textureSize(samplerCubeArrayShadow sampler, int lod);
#endif

#if __glslver__ >= 430
int textureQueryLevels(sampler1D sampler);
int textureQueryLevels(sampler2D sampler);
int textureQueryLevels(sampler3D sampler);
int textureQueryLevels(samplerCube sampler);
int textureQueryLevels(sampler1DArray sampler);
int textureQueryLevels(sampler2DArray sampler);
int textureQueryLevels(samplerCubeArray sampler);
int textureQueryLevels(sampler1DShadow sampler);
int textureQueryLevels(sampler2DShadow sampler);
int textureQueryLevels(samplerCubeShadow sampler);
int textureQueryLevels(sampler1DArrayShadow sampler);
int textureQueryLevels(sampler2DArrayShadow sampler);
int textureQueryLevels(samplerCubeArrayShadow sampler);
#endif

#if __glslver__ >= 450
int textureSamples(sampler2DMS sampler);
int textureSamples(sampler2DMSArray sampler);
#endif

#undef int

#if __glslver__ >= 120
mat2 outerProduct(vec2 c, vec2 r);
mat3 outerProduct(vec3 c, vec3 r);
mat4 outerProduct(vec4 c, vec4 r);
mat2x3 outerProduct(vec3 c, vec2 r);
mat3x2 outerProduct(vec2 c, vec3 r);
mat2x4 outerProduct(vec4 c, vec2 r);
mat4x2 outerProduct(vec2 c, vec4 r);
mat3x4 outerProduct(vec4 c, vec3 r);
mat4x3 outerProduct(vec3 c, vec4 r);
template <template <class t0> class v0, int n0>
matx<v0<float>, n0> transpose(matx<v0<float>, n0>);
#endif

#if __glslver__ >= 140
mat2 inverse(mat2 m);
mat3 inverse(mat3 m);
mat4 inverse(mat4 m);
#endif

#if __glslver__ >= 150
float determinant(mat2 m);
float determinant(mat3 m);
float determinant(mat4 m);
#endif

#if __glslver__ >= 400
double determinant(dmat2 m);
double determinant(dmat3 m);
double determinant(dmat4 m);
dmat2 inverse(dmat2 m);
dmat3 inverse(dmat3 m);
dmat4 inverse(dmat4 m);
dmat2 outerProduct(dvec2 c, dvec2 r);
dmat3 outerProduct(dvec3 c, dvec3 r);
dmat4 outerProduct(dvec4 c, dvec4 r);
dmat2x3 outerProduct(dvec3 c, dvec2 r);
dmat3x2 outerProduct(dvec2 c, dvec3 r);
dmat2x4 outerProduct(dvec4 c, dvec2 r);
dmat4x2 outerProduct(dvec2 c, dvec4 r);
dmat3x4 outerProduct(dvec4 c, dvec3 r);
dmat4x3 outerProduct(dvec3 c, dvec4 r);

template <template <class t0> class v0, int n0>
matx<v0<double>, n0> transpose(matx<v0<double>, n0>);

template <template <class t0> class v0, int n0>
matx<v0<double>, n0> matrixCompMult(matx<v0<double>, n0> x,
                                    matx<v0<double>, n0> y);
#endif

template <template <class t0> class v0, int n0>
matx<v0<float>, n0> matrixCompMult(matx<v0<float>, n0> x,
                                   matx<v0<float>, n0> y);
#define int int_

#if __glslver__ >= 400
int bitCount(int);
uint bitCount(uint);
ivec2 bitCount(ivec2);
uvec2 bitCount(uvec2);
ivec3 bitCount(ivec3);
uvec3 bitCount(uvec3);
ivec4 bitCount(ivec4);
uvec4 bitCount(uvec4);
int bitfieldExtract(int value, int offset, int bits);
uint bitfieldExtract(uint value, int offset, int bits);
ivec2 bitfieldExtract(ivec2 value, int offset, int bits);
uvec2 bitfieldExtract(uvec2 value, int offset, int bits);
ivec3 bitfieldExtract(ivec3 value, int offset, int bits);
uvec3 bitfieldExtract(uvec3 value, int offset, int bits);
ivec4 bitfieldExtract(ivec4 value, int offset, int bits);
uvec4 bitfieldExtract(uvec4 value, int offset, int bits);
int bitfieldInsert(int base, int insert, int offset, int bits);
uint bitfieldInsert(uint base, uint insert, int offset, int bits);
ivec2 bitfieldInsert(ivec2 base, ivec2 insert, int offset, int bits);
uvec2 bitfieldInsert(uvec2 base, uvec2 insert, int offset, int bits);
ivec3 bitfieldInsert(ivec3 base, ivec3 insert, int offset, int bits);
uvec3 bitfieldInsert(uvec3 base, uvec3 insert, int offset, int bits);
ivec4 bitfieldInsert(ivec4 base, ivec4 insert, int offset, int bits);
uvec4 bitfieldInsert(uvec4 base, uvec4 insert, int offset, int bits);
int bitfieldReverse(int);
uint bitfieldReverse(uint);
ivec2 bitfieldReverse(ivec2);
uvec2 bitfieldReverse(uvec2);
ivec3 bitfieldReverse(ivec3);
uvec3 bitfieldReverse(uvec3);
ivec4 bitfieldReverse(ivec4);
uvec4 bitfieldReverse(uvec4);
int findLSB(int);
uint findLSB(uint);
ivec2 findLSB(ivec2);
uvec2 findLSB(uvec2);
ivec3 findLSB(ivec3);
uvec3 findLSB(uvec3);
ivec4 findLSB(ivec4);
uvec4 findLSB(uvec4);
int findMSB(int);
uint findMSB(uint);
ivec2 findMSB(ivec2);
uvec2 findMSB(uvec2);
ivec3 findMSB(ivec3);
uvec3 findMSB(uvec3);
ivec4 findMSB(ivec4);
uvec4 findMSB(uvec4);
uint uaddCarry(uint x, uint y, uint carry);
uvec2 uaddCarry(uvec2 x, uvec2 y, uvec2 carry);
uvec3 uaddCarry(uvec3 x, uvec3 y, uvec3 carry);
uvec4 uaddCarry(uvec4 x, uvec4 y, uvec4 carry);
uint umulExtended(uint x, uint y, uint msb, uint lsb);
uvec2 umulExtended(uvec2 x, uvec2 y, uvec2 msb, uvec2 lsb);
uvec3 umulExtended(uvec3 x, uvec3 y, uvec3 msb, uvec3 lsb);
uvec4 umulExtended(uvec4 x, uvec4 y, uvec4 msb, uvec4 lsb);
int imulExtended(int x, int y, int msb, int lsb);
ivec2 imulExtended(ivec2 x, ivec2 y, ivec2 msb, ivec2 lsb);
ivec3 imulExtended(ivec3 x, ivec3 y, ivec3 msb, ivec3 lsb);
ivec4 imulExtended(ivec4 x, ivec4 y, ivec4 msb, ivec4 lsb);
uint usubBorrow(uint x, uint y, uint borrow);
uvec2 usubBorrow(uvec2 x, uvec2 y, uvec2 borrow);
uvec3 usubBorrow(uvec3 x, uvec3 y, uvec3 borrow);
uvec4 usubBorrow(uvec4 x, uvec4 y, uvec4 borrow);
#endif

#if __glslver__ >= 150
void EmitVertex();
void EndPrimitive();
#endif

#if __glslver__ >= 400
void EmitStreamVertex(int stream);
void EndStreamPrimitive(int stream);
#endif

#if __glslver__ >= 420
struct image1D {};
struct image2D {};
struct image3D {};
struct image2DRect {};
struct imageCube {};
struct bufferImage {};
struct image1DArray {};
struct image2DArray {};
struct imageCubeArray {};
struct image2DMS {};
struct image2DMSArray {};

uint imageAtomicAdd(image1D image, int P, uint data);
uint imageAtomicAdd(image2D image, ivec2 P, uint data);
uint imageAtomicAdd(image3D image, ivec3 P, uint data);
uint imageAtomicAdd(image2DRect image, ivec2 P, uint data);
uint imageAtomicAdd(imageCube image, ivec3 P, uint data);
uint imageAtomicAdd(bufferImage image, int P, uint data);
uint imageAtomicAdd(image1DArray image, ivec2 P, uint data);
uint imageAtomicAdd(image2DArray image, ivec3 P, uint data);
uint imageAtomicAdd(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicAdd(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicAdd(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicAdd(image1D image, int P, int data);
int imageAtomicAdd(image2D image, ivec2 P, int data);
int imageAtomicAdd(image3D image, ivec3 P, int data);
int imageAtomicAdd(image2DRect image, ivec2 P, int data);
int imageAtomicAdd(imageCube image, ivec3 P, int data);
int imageAtomicAdd(bufferImage image, int P, int data);
int imageAtomicAdd(image1DArray image, ivec2 P, int data);
int imageAtomicAdd(image2DArray image, ivec3 P, int data);
int imageAtomicAdd(imageCubeArray image, ivec3 P, int data);
int imageAtomicAdd(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicAdd(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicAnd(image1D image, int P, uint data);
uint imageAtomicAnd(image2D image, ivec2 P, uint data);
uint imageAtomicAnd(image3D image, ivec3 P, uint data);
uint imageAtomicAnd(image2DRect image, ivec2 P, uint data);
uint imageAtomicAnd(imageCube image, ivec3 P, uint data);
uint imageAtomicAnd(bufferImage image, int P, uint data);
uint imageAtomicAnd(image1DArray image, ivec2 P, uint data);
uint imageAtomicAnd(image2DArray image, ivec3 P, uint data);
uint imageAtomicAnd(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicAnd(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicAnd(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicAnd(image1D image, int P, int data);
int imageAtomicAnd(image2D image, ivec2 P, int data);
int imageAtomicAnd(image3D image, ivec3 P, int data);
int imageAtomicAnd(image2DRect image, ivec2 P, int data);
int imageAtomicAnd(imageCube image, ivec3 P, int data);
int imageAtomicAnd(bufferImage image, int P, int data);
int imageAtomicAnd(image1DArray image, ivec2 P, int data);
int imageAtomicAnd(image2DArray image, ivec3 P, int data);
int imageAtomicAnd(imageCubeArray image, ivec3 P, int data);
int imageAtomicAnd(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicAnd(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicCompSwap(image1D image, int P, uint compare, uint data);
uint imageAtomicCompSwap(image2D image, ivec2 P, uint compare, uint data);
uint imageAtomicCompSwap(image3D image, ivec3 P, uint compare, uint data);
uint imageAtomicCompSwap(image2DRect image, ivec2 P, uint compare, uint data);
uint imageAtomicCompSwap(imageCube image, ivec3 P, uint compare, uint data);
uint imageAtomicCompSwap(bufferImage image, int P, uint compare, uint data);
uint imageAtomicCompSwap(image1DArray image, ivec2 P, uint compare, uint data);
uint imageAtomicCompSwap(image2DArray image, ivec3 P, uint compare, uint data);
uint imageAtomicCompSwap(imageCubeArray image, ivec3 P, uint compare,
                         uint data);
uint imageAtomicCompSwap(image2DMS image, ivec2 P, int sample, uint compare,
                         uint data);
uint imageAtomicCompSwap(image2DMSArray image, ivec3 P, int sample,
                         uint compare, uint data);
int imageAtomicCompSwap(image1D image, int P, int compare, int data);
int imageAtomicCompSwap(image2D image, ivec2 P, int compare, int data);
int imageAtomicCompSwap(image3D image, ivec3 P, int compare, int data);
int imageAtomicCompSwap(image2DRect image, ivec2 P, int compare, int data);
int imageAtomicCompSwap(imageCube image, ivec3 P, int compare, int data);
int imageAtomicCompSwap(bufferImage image, int P, int compare, int data);
int imageAtomicCompSwap(image1DArray image, ivec2 P, int compare, int data);
int imageAtomicCompSwap(image2DArray image, ivec3 P, int compare, int data);
int imageAtomicCompSwap(imageCubeArray image, ivec3 P, int compare, int data);
int imageAtomicCompSwap(image2DMS image, ivec2 P, int sample, int compare,
                        int data);
int imageAtomicCompSwap(image2DMSArray image, ivec3 P, int sample, int compare,
                        int data);

uint imageAtomicExchange(image1D image, int P, uint data);
uint imageAtomicExchange(image2D image, ivec2 P, uint data);
uint imageAtomicExchange(image3D image, ivec3 P, uint data);
uint imageAtomicExchange(image2DRect image, ivec2 P, uint data);
uint imageAtomicExchange(imageCube image, ivec3 P, uint data);
uint imageAtomicExchange(bufferImage image, int P, uint data);
uint imageAtomicExchange(image1DArray image, ivec2 P, uint data);
uint imageAtomicExchange(image2DArray image, ivec3 P, uint data);
uint imageAtomicExchange(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicExchange(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicExchange(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicExchange(image1D image, int P, int data);
int imageAtomicExchange(image2D image, ivec2 P, int data);
int imageAtomicExchange(image3D image, ivec3 P, int data);
int imageAtomicExchange(image2DRect image, ivec2 P, int data);
int imageAtomicExchange(imageCube image, ivec3 P, int data);
int imageAtomicExchange(bufferImage image, int P, int data);
int imageAtomicExchange(image1DArray image, ivec2 P, int data);
int imageAtomicExchange(image2DArray image, ivec3 P, int data);
int imageAtomicExchange(imageCubeArray image, ivec3 P, int data);
int imageAtomicExchange(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicExchange(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicMax(image1D image, int P, uint data);
uint imageAtomicMax(image2D image, ivec2 P, uint data);
uint imageAtomicMax(image3D image, ivec3 P, uint data);
uint imageAtomicMax(image2DRect image, ivec2 P, uint data);
uint imageAtomicMax(imageCube image, ivec3 P, uint data);
uint imageAtomicMax(bufferImage image, int P, uint data);
uint imageAtomicMax(image1DArray image, ivec2 P, uint data);
uint imageAtomicMax(image2DArray image, ivec3 P, uint data);
uint imageAtomicMax(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicMax(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicMax(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicMax(image1D image, int P, int data);
int imageAtomicMax(image2D image, ivec2 P, int data);
int imageAtomicMax(image3D image, ivec3 P, int data);
int imageAtomicMax(image2DRect image, ivec2 P, int data);
int imageAtomicMax(imageCube image, ivec3 P, int data);
int imageAtomicMax(bufferImage image, int P, int data);
int imageAtomicMax(image1DArray image, ivec2 P, int data);
int imageAtomicMax(image2DArray image, ivec3 P, int data);
int imageAtomicMax(imageCubeArray image, ivec3 P, int data);
int imageAtomicMax(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicMax(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicMin(image1D image, int P, uint data);
uint imageAtomicMin(image2D image, ivec2 P, uint data);
uint imageAtomicMin(image3D image, ivec3 P, uint data);
uint imageAtomicMin(image2DRect image, ivec2 P, uint data);
uint imageAtomicMin(imageCube image, ivec3 P, uint data);
uint imageAtomicMin(bufferImage image, int P, uint data);
uint imageAtomicMin(image1DArray image, ivec2 P, uint data);
uint imageAtomicMin(image2DArray image, ivec3 P, uint data);
uint imageAtomicMin(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicMin(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicMin(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicMin(image1D image, int P, int data);
int imageAtomicMin(image2D image, ivec2 P, int data);
int imageAtomicMin(image3D image, ivec3 P, int data);
int imageAtomicMin(image2DRect image, ivec2 P, int data);
int imageAtomicMin(imageCube image, ivec3 P, int data);
int imageAtomicMin(bufferImage image, int P, int data);
int imageAtomicMin(image1DArray image, ivec2 P, int data);
int imageAtomicMin(image2DArray image, ivec3 P, int data);
int imageAtomicMin(imageCubeArray image, ivec3 P, int data);
int imageAtomicMin(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicMin(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicOr(image1D image, int P, uint data);
uint imageAtomicOr(image2D image, ivec2 P, uint data);
uint imageAtomicOr(image3D image, ivec3 P, uint data);
uint imageAtomicOr(image2DRect image, ivec2 P, uint data);
uint imageAtomicOr(imageCube image, ivec3 P, uint data);
uint imageAtomicOr(bufferImage image, int P, uint data);
uint imageAtomicOr(image1DArray image, ivec2 P, uint data);
uint imageAtomicOr(image2DArray image, ivec3 P, uint data);
uint imageAtomicOr(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicOr(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicOr(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicOr(image1D image, int P, int data);
int imageAtomicOr(image2D image, ivec2 P, int data);
int imageAtomicOr(image3D image, ivec3 P, int data);
int imageAtomicOr(image2DRect image, ivec2 P, int data);
int imageAtomicOr(imageCube image, ivec3 P, int data);
int imageAtomicOr(bufferImage image, int P, int data);
int imageAtomicOr(image1DArray image, ivec2 P, int data);
int imageAtomicOr(image2DArray image, ivec3 P, int data);
int imageAtomicOr(imageCubeArray image, ivec3 P, int data);
int imageAtomicOr(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicOr(image2DMSArray image, ivec3 P, int sample, int data);

uint imageAtomicXor(image1D image, int P, uint data);
uint imageAtomicXor(image2D image, ivec2 P, uint data);
uint imageAtomicXor(image3D image, ivec3 P, uint data);
uint imageAtomicXor(image2DRect image, ivec2 P, uint data);
uint imageAtomicXor(imageCube image, ivec3 P, uint data);
uint imageAtomicXor(bufferImage image, int P, uint data);
uint imageAtomicXor(image1DArray image, ivec2 P, uint data);
uint imageAtomicXor(image2DArray image, ivec3 P, uint data);
uint imageAtomicXor(imageCubeArray image, ivec3 P, uint data);
uint imageAtomicXor(image2DMS image, ivec2 P, int sample, uint data);
uint imageAtomicXor(image2DMSArray image, ivec3 P, int sample, uint data);
int imageAtomicXor(image1D image, int P, int data);
int imageAtomicXor(image2D image, ivec2 P, int data);
int imageAtomicXor(image3D image, ivec3 P, int data);
int imageAtomicXor(image2DRect image, ivec2 P, int data);
int imageAtomicXor(imageCube image, ivec3 P, int data);
int imageAtomicXor(bufferImage image, int P, int data);
int imageAtomicXor(image1DArray image, ivec2 P, int data);
int imageAtomicXor(image2DArray image, ivec3 P, int data);
int imageAtomicXor(imageCubeArray image, ivec3 P, int data);
int imageAtomicXor(image2DMS image, ivec2 P, int sample, int data);
int imageAtomicXor(image2DMSArray image, ivec3 P, int sample, int data);

vec4 imageLoad(image1D image, int P);
vec4 imageLoad(image2D image, ivec2 P);
vec4 imageLoad(image3D image, ivec3 P);
vec4 imageLoad(image2DRect image, ivec2 P);
vec4 imageLoad(imageCube image, ivec3 P);
vec4 imageLoad(bufferImage image, int P);
vec4 imageLoad(image1DArray image, ivec2 P);
vec4 imageLoad(image2DArray image, ivec3 P);
vec4 imageLoad(imageCubeArray image, ivec3 P);
vec4 imageLoad(image2DMS image, ivec2 P, int sample);
vec4 imageLoad(image2DMSArray image, ivec3 P, int sample);

void imageStore(image1D image, int P, vec4 data);
void imageStore(image2D image, ivec2 P, vec4 data);
void imageStore(image3D image, ivec3 P, vec4 data);
void imageStore(image2DRect image, ivec2 P, vec4 data);
void imageStore(imageCube image, ivec3 P, vec4 data);
void imageStore(bufferImage image, int P, vec4 data);
void imageStore(image1DArray image, ivec2 P, vec4 data);
void imageStore(image2DArray image, ivec3 P, vec4 data);
void imageStore(imageCubeArray image, ivec3 P, vec4 data);
void imageStore(image2DMS image, ivec2 P, int sample, vec4 data);
void imageStore(image2DMSArray image, ivec3 P, int sample, vec4 data);
#endif

#if __glslver__ >= 430
int imageSize(image1D image);
ivec2 imageSize(image2D image);
ivec3 imageSize(image3D image);
ivec2 imageSize(imageCube image);
ivec3 imageSize(imageCubeArray image);
ivec2 imageSize(image2DRect image);
ivec2 imageSize(image1DArray image);
ivec3 imageSize(image2DArray image);
int imageSize(bufferImage image);
ivec2 imageSize(image2DMS image);
ivec3 imageSize(image2DMSArray image);
#endif

#if __glslver__ >= 450
int imageAtomicExchange(image1D image, int P, float data);
int imageAtomicExchange(image2D image, ivec2 P, float data);
int imageAtomicExchange(image3D image, ivec3 P, float data);
int imageAtomicExchange(image2DRect image, ivec2 P, float data);
int imageAtomicExchange(imageCube image, ivec3 P, float data);
int imageAtomicExchange(bufferImage image, int P, float data);
int imageAtomicExchange(image1DArray image, ivec2 P, float data);
int imageAtomicExchange(image2DArray image, ivec3 P, float data);
int imageAtomicExchange(imageCubeArray image, ivec3 P, float data);
int imageAtomicExchange(image2DMS image, ivec2 P, int sample, float data);
int imageAtomicExchange(image2DMSArray image, ivec3 P, int sample, float data);
int imageSamples(image2DMS image);
int imageSamples(image2DMSArray image);
#endif

#if __glslver__ >= 420
struct atomic_uint {};
uint atomicCounter(atomic_uint c);
uint atomicCounterDecrement(atomic_uint c);
uint atomicCounterIncrement(atomic_uint c);
#endif

#if __glslver__ >= 430
int atomicAdd(int mem, int data);
uint atomicAdd(uint mem, uint data);
int atomicAnd(int mem, int data);
uint atomicAnd(uint mem, uint data);
int atomicCompSwap(int mem, uint compare, uint data);
uint atomicCompSwap(uint mem, uint compare, uint data);
int atomicExchange(int mem, int data);
uint atomicExchange(uint mem, uint data);
int atomicMax(int mem, int data);
uint atomicMax(uint mem, uint data);
int atomicMin(int mem, int data);
uint atomicMin(uint mem, uint data);
int atomicOr(int mem, int data);
uint atomicOr(uint mem, uint data);
int atomicXor(int mem, int data);
uint atomicXor(uint mem, uint data);
#endif

#if __glslver__ >= 400
void barrier();
void memoryBarrier();
#endif

#if __glslver__ >= 430
void groupMemoryBarrier();
void memoryBarrierAtomicCounter();
void memoryBarrierBuffer();
void memoryBarrierImage();
void memoryBarrierShared();
#endif

// Vertex stage
extern const int gl_VertexID;
extern const int gl_InstanceID;
#if __glslver__ >= 460
extern const int gl_DrawID;
extern const int gl_BaseVertex;
extern const int gl_BaseInstance;
#endif

vec4 gl_Position;
float gl_PointSize;
extern float gl_ClipDistance[];

// Tesselation stage
extern const int gl_PatchVerticesIn;
extern const int gl_PrimitiveID;
extern const int gl_InvocationID;
extern const vec3 gl_TessCoord; // Evaluation only

struct {
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
} extern const gl_in[];

float gl_TessLevelOuter[4];
float gl_TessLevelInner[2];

struct {
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
} extern gl_out[];

// Geometry stage
extern const int gl_PrimitiveIDIn;
#if __glslver__ >= 400
extern const int gl_InvocationID;
#endif

int gl_Layer;
#if __glslver__ >= 410
int gl_ViewportIndex;
#endif

// Fragment shader
extern const vec4 gl_FragCoord;
extern const bool gl_FrontFacing;
extern const vec2 gl_PointCoord;
extern const int gl_SampleID;
extern const vec2 gl_SamplePosition;
extern const int gl_SampleMaskIn[];

float gl_FragDepth;
extern int gl_SampleMask[];

// Compute shader
extern const uvec3 gl_NumWorkGroups;
extern const uvec3 gl_WorkGroupID;
extern const uvec3 gl_LocalInvocationID;
extern const uvec3 gl_GlobalInvocationID;
extern const uint gl_LocalInvocationIndex;

#if __glslver__ >= 430
constexpr uvec3 gl_WorkGroupSize(16);
#endif

struct gl_DepthRangeParameters {
  float near;
  float far;
  float diff;
};

gl_DepthRangeParameters gl_DepthRange;

#if __glslver__ >= 400
int gl_NumSamples;
#endif

bool discard;
#endif
