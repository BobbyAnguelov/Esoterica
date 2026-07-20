#ifndef UFBX_UFBX_MATH_H_INCLUDED
#define UFBX_UFBX_MATH_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef ufbx_math_abi
    #if defined(UFBX_STATIC)
        #define ufbx_math_abi static
    #else
        #define ufbx_math_abi
    #endif
#endif

ufbx_math_abi double ufbx_sqrt(double x);
ufbx_math_abi double ufbx_sin(double x);
ufbx_math_abi double ufbx_cos(double x);
ufbx_math_abi double ufbx_tan(double x);
ufbx_math_abi double ufbx_asin(double x);
ufbx_math_abi double ufbx_acos(double x);
ufbx_math_abi double ufbx_atan(double x);
ufbx_math_abi double ufbx_atan2(double y, double x);
ufbx_math_abi double ufbx_pow(double x, double y);
ufbx_math_abi double ufbx_fmin(double a, double b);
ufbx_math_abi double ufbx_fmax(double a, double b);
ufbx_math_abi double ufbx_fabs(double x);
ufbx_math_abi double ufbx_copysign(double x, double y);
ufbx_math_abi double ufbx_nextafter(double x, double y);
ufbx_math_abi double ufbx_rint(double x);
ufbx_math_abi double ufbx_floor(double x);
ufbx_math_abi double ufbx_ceil(double x);
ufbx_math_abi int ufbx_isnan(double x);

#if defined(__cplusplus)
}
#endif

#endif
