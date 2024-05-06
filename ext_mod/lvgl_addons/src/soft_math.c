/* origin: FreeBSD /usr/src/lib/msun/src/e_powf.c */
/*
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */


#include <math.h>
#include <stdint.h>

#include "../include/soft_math.h"


#ifdef MP_SOFT_MATH
    /* Get a 32 bit int from a float.  */
    #define GET_FLOAT_WORD(w,d)                       \
    do {                                              \
      union {float f; uint32_t i;} __u;               \
      __u.f = (d);                                    \
      (w) = __u.i;                                    \
    } while (0)


    /* Set a float from a 32 bit int.  */
    #define SET_FLOAT_WORD(d,w)                       \
    do {                                              \
      union {float f; uint32_t i;} __u;               \
      __u.i = (w);                                    \
      (d) = __u.f;                                    \
    } while (0)


    float soft_fabsf(float x)
    {
        union {float f; uint32_t i;} u = {x};
        u.i &= 0x7fffffff;
        return u.f;
    }


    float soft_sqrtf(float x)
    {
        static const float tiny = 1.0e-30f;

        float z;
        int32_t sign = (int)0x80000000;
        int32_t ix,s,q,m,t,i;
        uint32_t r;

        GET_FLOAT_WORD(ix, x);

        /* take care of Inf and NaN */
        if ((ix&0x7f800000) == 0x7f800000)
            return x*x + x; /* sqrt(NaN)=NaN, sqrt(+inf)=+inf, sqrt(-inf)=sNaN */

        /* take care of zero */
        if (ix <= 0) {
            if ((ix&~sign) == 0)
                return x;  /* sqrt(+-0) = +-0 */
            if (ix < 0)
                return (x-x)/(x-x);  /* sqrt(-ve) = sNaN */
        }
        /* normalize x */
        m = ix>>23;
        if (m == 0) {  /* subnormal x */
            for (i = 0; (ix&0x00800000) == 0; i++)
                ix<<=1;
            m -= i - 1;
        }
        m -= 127;  /* unbias exponent */
        ix = (ix&0x007fffff)|0x00800000;
        if (m&1)  /* odd m, double x to make it even */
            ix += ix;
        m >>= 1;  /* m = [m/2] */

        /* generate sqrt(x) bit by bit */
        ix += ix;
        q = s = 0;       /* q = sqrt(x) */
        r = 0x01000000;  /* r = moving bit from right to left */

        while (r != 0) {
            t = s + r;
            if (t <= ix) {
                s = t+r;
                ix -= t;
                q += r;
            }
            ix += ix;
            r >>= 1;
        }

        /* use floating add to find out rounding direction */
        if (ix != 0) {
            z = 1.0f - tiny; /* raise inexact flag */
            if (z >= 1.0f) {
                z = 1.0f + tiny;
                if (z > 1.0f)
                    q += 2;
                else
                    q += q & 1;
            }
        }
        ix = (q>>1) + 0x3f000000;
        ix += m << 23;
        SET_FLOAT_WORD(z, ix);
        return z;
    }

    // Approximates atan2(y, x) normalized to the [0,4) range
    // with a maximum error of 0.1620 degrees
    float soft_atan2( float y, float x )
    {
        static const uint32_t sign_mask = 0x80000000;
        static const float b = 0.596227f;

        // Extract the sign bits
        uint32_t ux_s  = sign_mask & (uint32_t)x;
        uint32_t uy_s  = sign_mask & (uint32_t)y;

        // Determine the quadrant offset
        float q = (float)( ( ~ux_s & uy_s ) >> 29 | ux_s >> 30 );

        // Calculate the arctangent in the first quadrant
        float bxy_a = (float)fabs( b * x * y );
        float num = bxy_a + y * y;
        float atan_1q =  num / ( x * x + bxy_a + num );

        // Translate it to the proper quadrant
        uint32_t uatan_2q = (ux_s ^ uy_s) | (uint32_t)atan_1q;
        return q + (float)uatan_2q;
    }

    float soft_powf(float x, float y)
    {

        static const float
        bp[]   = {1.0f, 1.5f,},
        dp_h[] = { 0.0, 5.84960938e-01f,}, /* 0x3f15c000 */
        dp_l[] = { 0.0, 1.56322085e-06f,}, /* 0x35d1cfdc */
        two24  =  16777216.0f,  /* 0x4b800000 */
        huge   =  1.0e30f,
        tiny   =  1.0e-30f,
        /* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
        L1 =  6.0000002384e-01f, /* 0x3f19999a */
        L2 =  4.2857143283e-01f, /* 0x3edb6db7 */
        L3 =  3.3333334327e-01f, /* 0x3eaaaaab */
        L4 =  2.7272811532e-01f, /* 0x3e8ba305 */
        L5 =  2.3066075146e-01f, /* 0x3e6c3255 */
        L6 =  2.0697501302e-01f, /* 0x3e53f142 */
        P1 =  1.6666667163e-01f, /* 0x3e2aaaab */
        P2 = -2.7777778450e-03f, /* 0xbb360b61 */
        P3 =  6.6137559770e-05f, /* 0x388ab355 */
        P4 = -1.6533901999e-06f, /* 0xb5ddea0e */
        P5 =  4.1381369442e-08f, /* 0x3331bb4c */
        lg2     =  6.9314718246e-01f, /* 0x3f317218 */
        lg2_h   =  6.93145752e-01f,   /* 0x3f317200 */
        lg2_l   =  1.42860654e-06f,   /* 0x35bfbe8c */
        ovt     =  4.2995665694e-08f, /* -(128-log2(ovfl+.5ulp)) */
        cp      =  9.6179670095e-01f, /* 0x3f76384f =2/(3ln2) */
        cp_h    =  9.6191406250e-01f, /* 0x3f764000 =12b cp */
        cp_l    = -1.1736857402e-04f, /* 0xb8f623c6 =tail of cp_h */
        ivln2   =  1.4426950216e+00f, /* 0x3fb8aa3b =1/ln2 */
        ivln2_h =  1.4426879883e+00f, /* 0x3fb8aa00 =16b 1/ln2*/
        ivln2_l =  7.0526075433e-06f; /* 0x36eca570 =1/ln2 tail*/


        float z,ax,z_h,z_l,p_h,p_l;
        float y1,t1,t2,r,s,sn,t,u,v,w;
        int32_t i,j,k,yisint,n;
        int32_t hx,hy,ix,iy,is;

        GET_FLOAT_WORD(hx, x);
        GET_FLOAT_WORD(hy, y);
        ix = hx & 0x7fffffff;
        iy = hy & 0x7fffffff;

        /* x**0 = 1, even if x is NaN */
        if (iy == 0)
            return 1.0f;
        /* 1**y = 1, even if y is NaN */
        if (hx == 0x3f800000)
            return 1.0f;
        /* NaN if either arg is NaN */
        if (ix > 0x7f800000 || iy > 0x7f800000)
            return x + y;

        /* determine if y is an odd int when x < 0
         * yisint = 0       ... y is not an integer
         * yisint = 1       ... y is an odd int
         * yisint = 2       ... y is an even int
         */
        yisint  = 0;
        if (hx < 0) {
            if (iy >= 0x4b800000)
                yisint = 2; /* even integer y */
            else if (iy >= 0x3f800000) {
                k = (iy>>23) - 0x7f;         /* exponent */
                j = iy>>(23-k);
                if ((j<<(23-k)) == iy)
                    yisint = 2 - (j & 1);
            }
        }

        /* special value of y */
        if (iy == 0x7f800000) {  /* y is +-inf */
            if (ix == 0x3f800000)      /* (-1)**+-inf is 1 */
                return 1.0f;
            else if (ix > 0x3f800000)  /* (|x|>1)**+-inf = inf,0 */
                return hy >= 0 ? y : 0.0f;
            else if (ix != 0)          /* (|x|<1)**+-inf = 0,inf if x!=0 */
                return hy >= 0 ? 0.0f: -y;
        }
        if (iy == 0x3f800000)    /* y is +-1 */
            return hy >= 0 ? x : 1.0f/x;
        if (hy == 0x40000000)    /* y is 2 */
            return x*x;
        if (hy == 0x3f000000) {  /* y is  0.5 */
            if (hx >= 0)     /* x >= +0 */
                return soft_sqrtf(x);
        }

        ax = soft_fabsf(x);
        /* special value of x */
        if (ix == 0x7f800000 || ix == 0 || ix == 0x3f800000) { /* x is +-0,+-inf,+-1 */
            z = ax;
            if (hy < 0)  /* z = (1/|x|) */
                z = 1.0f/z;
            if (hx < 0) {
                if (((ix-0x3f800000)|yisint) == 0) {
                    z = (z-z)/(z-z); /* (-1)**non-int is NaN */
                } else if (yisint == 1)
                    z = -z;          /* (x<0)**odd = -(|x|**odd) */
            }
            return z;
        }

        sn = 1.0f; /* sign of result */
        if (hx < 0) {
            if (yisint == 0) /* (x<0)**(non-int) is NaN */
                return (x-x)/(x-x);
            if (yisint == 1) /* (x<0)**(odd int) */
                sn = -1.0f;
        }

        /* |y| is huge */
        if (iy > 0x4d000000) { /* if |y| > 2**27 */
            /* over/underflow if x is not close to one */
            if (ix < 0x3f7ffff8)
                return hy < 0 ? sn*huge*huge : sn*tiny*tiny;
            if (ix > 0x3f800007)
                return hy > 0 ? sn*huge*huge : sn*tiny*tiny;
            /* now |1-x| is tiny <= 2**-20, suffice to compute
               log(x) by x-x^2/2+x^3/3-x^4/4 */
            t = ax - 1;     /* t has 20 trailing zeros */
            w = (t*t)*(0.5f - t*(0.333333333333f - t*0.25f));
            u = ivln2_h*t;  /* ivln2_h has 16 sig. bits */
            v = t*ivln2_l - w*ivln2;
            t1 = u + v;
            GET_FLOAT_WORD(is, t1);
            SET_FLOAT_WORD(t1, is & 0xfffff000);
            t2 = v - (t1-u);
        } else {
            float s2,s_h,s_l,t_h,t_l;
            n = 0;
            /* take care subnormal number */
            if (ix < 0x00800000) {
                ax *= two24;
                n -= 24;
                GET_FLOAT_WORD(ix, ax);
            }
            n += ((ix)>>23) - 0x7f;
            j = ix & 0x007fffff;
            /* determine interval */
            ix = j | 0x3f800000;     /* normalize ix */
            if (j <= 0x1cc471)       /* |x|<sqrt(3/2) */
                k = 0;
            else if (j < 0x5db3d7)   /* |x|<sqrt(3)   */
                k = 1;
            else {
                k = 0;
                n += 1;
                ix -= 0x00800000;
            }
            SET_FLOAT_WORD(ax, ix);

            /* compute s = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
            u = ax - bp[k];   /* bp[0]=1.0, bp[1]=1.5 */
            v = 1.0f/(ax+bp[k]);
            s = u*v;
            s_h = s;
            GET_FLOAT_WORD(is, s_h);
            SET_FLOAT_WORD(s_h, is & 0xfffff000);
            /* t_h=ax+bp[k] High */
            is = ((ix>>1) & 0xfffff000) | 0x20000000;
            SET_FLOAT_WORD(t_h, is + 0x00400000 + (k<<21));
            t_l = ax - (t_h - bp[k]);
            s_l = v*((u - s_h*t_h) - s_h*t_l);
            /* compute log(ax) */
            s2 = s*s;
            r = s2*s2*(L1+s2*(L2+s2*(L3+s2*(L4+s2*(L5+s2*L6)))));
            r += s_l*(s_h+s);
            s2 = s_h*s_h;
            t_h = 3.0f + s2 + r;
            GET_FLOAT_WORD(is, t_h);
            SET_FLOAT_WORD(t_h, is & 0xfffff000);
            t_l = r - ((t_h - 3.0f) - s2);
            /* u+v = s*(1+...) */
            u = s_h*t_h;
            v = s_l*t_h + t_l*s;
            /* 2/(3log2)*(s+...) */
            p_h = u + v;
            GET_FLOAT_WORD(is, p_h);
            SET_FLOAT_WORD(p_h, is & 0xfffff000);
            p_l = v - (p_h - u);
            z_h = cp_h*p_h;  /* cp_h+cp_l = 2/(3*log2) */
            z_l = cp_l*p_h + p_l*cp+dp_l[k];
            /* log2(ax) = (s+..)*2/(3*log2) = n + dp_h + z_h + z_l */
            t = (float)n;
            t1 = (((z_h + z_l) + dp_h[k]) + t);
            GET_FLOAT_WORD(is, t1);
            SET_FLOAT_WORD(t1, is & 0xfffff000);
            t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
        }

        /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
        GET_FLOAT_WORD(is, y);
        SET_FLOAT_WORD(y1, is & 0xfffff000);
        p_l = (y-y1)*t1 + y*t2;
        p_h = y1*t1;
        z = p_l + p_h;
        GET_FLOAT_WORD(j, z);
        if (j > 0x43000000)          /* if z > 128 */
            return sn*huge*huge;  /* overflow */
        else if (j == 0x43000000) {  /* if z == 128 */
            if (p_l + ovt > z - p_h)
                return sn*huge*huge;  /* overflow */
        } else if ((j&0x7fffffff) > 0x43160000)  /* z < -150 */
            return sn*tiny*tiny;  /* underflow */
        else if (j == 0xc3160000) {  /* z == -150 */
            if (p_l <= z-p_h)
                return sn*tiny*tiny;  /* underflow */
        }
        /*
         * compute 2**(p_h+p_l)
         */
        i = j & 0x7fffffff;
        k = (i>>23) - 0x7f;
        n = 0;
        if (i > 0x3f000000) {   /* if |z| > 0.5, set n = [z+0.5] */
            n = j + (0x00800000>>(k+1));
            k = ((n&0x7fffffff)>>23) - 0x7f;  /* new k for n */
            SET_FLOAT_WORD(t, n & ~(0x007fffff>>k));
            n = ((n&0x007fffff)|0x00800000)>>(23-k);
            if (j < 0)
                n = -n;
            p_h -= t;
        }
        t = p_l + p_h;
        GET_FLOAT_WORD(is, t);
        SET_FLOAT_WORD(t, is & 0xffff8000);
        u = t*lg2_h;
        v = (p_l-(t-p_h))*lg2 + t*lg2_l;
        z = u + v;
        w = v - (z - u);
        t = z*z;
        t1 = z - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
        r = (z*t1)/(t1-2.0f) - (w+z*w);
        z = 1.0f - (r - z);
        GET_FLOAT_WORD(j, z);
        j += n<<23;
        if ((j>>23) <= 0)  /* subnormal output */
            z = soft_scalbnf(z, n);
        else
            SET_FLOAT_WORD(z, j);
        return sn*z;
    }


    float soft_scalbnf(float x, int n)
    {
        union {float f; uint32_t i;} u;
        float_t y = x;

        if (n > 127) {
            y *= 0x1p127f;
            n -= 127;
            if (n > 127) {
                y *= 0x1p127f;
                n -= 127;
                if (n > 127)
                    n = 127;
            }
        } else if (n < -126) {
            y *= 0x1p-126f;
            n += 126;
            if (n < -126) {
                y *= 0x1p-126f;
                n += 126;
                if (n < -126)
                    n = -126;
            }
        }
        u.i = (uint32_t)(0x7f+n)<<23;
        x = y * u.f;
        return x;
    }
#endif