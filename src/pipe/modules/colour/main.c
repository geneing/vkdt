#include "../crop/gaussian_elimination.h" // TODO: clean up and move to core/api headers
#include "modules/api.h"
#include <math.h>
#include <stdlib.h>


static inline void
clip_poly(
    const float *p,        // pointer to polygon points
    uint32_t     p_cnt,    // number of corners
    const float *w,        // constant white
    float       *v)        // point potentially outside, will clip this to p
{
  for(int i=0;i<p_cnt;i++)
  {
    float vp0 [] = {v[0]-p[2*i+0], v[1]-p[2*i+1]};
    float p1p0[] = {p[2*i+2+0]-p[2*i+0], p[2*i+2+1]-p[2*i+1]};
    float n[] = {p1p0[1], -p1p0[0]};
    float dotv = n[0]*vp0[0] + n[1]*vp0[1];
    if(dotv <= 0) continue; // inside
    float dotvw = n[0]*(v[0]-w[0]) + n[1]*(v[1]-w[1]);
    float dotpw = n[0]*(p[2*i]-w[0]) + n[1]*(p[2*i+1]-w[1]);
    float t = dotpw/dotvw;
    v[0] = w[0] + t * (v[0] - w[0]);
    v[1] = w[1] + t * (v[1] - w[1]);
  }
}


// clamp to spectral locus:
static inline void clip_spectral_locus(float *v)
{
  // manually picked subset of points on the spectral locus transformed to
  // rec2020 r/(r+g+b), b/(r+g+b). the points have been picked by walking the
  // wavelength domain in 5nm intervals and computing XYZ chromaticity
  // coordinates by integrating the 1931 2deg CMF.
  static const float gamut_rec2020_rb[] =
  {
    0.119264, 1.0047,
    0.0352311, 1.01002,
    -0.0587504, 0.957361,
    -0.166419, 0.817868,
    -0.269602, 0.55565,
    -0.300098, 0.281734,
    -0.228864, 0.0937406,
    -0.0263976, 0.00459292,
    0.230836, -0.0158645,
    1.051, 0.00138612,
    0.119264, 1.0047,        // wrap around point
  };
  const int cnt = sizeof(gamut_rec2020_rb) / sizeof(gamut_rec2020_rb[0]) / 2 - 1;

  const float white[] = {0.33333f, 0.33333f};
  clip_poly(gamut_rec2020_rb, cnt, white, v);

#if 0 // DEBUG
  srand48(666);
  for(int k=0;k<20;k++)
  {
    float v[] = {drand48() * 1.6 - 0.4, -0.2 + 1.4*drand48()};
    fprintf(stdout, "%g %g\n", v[0], v[1]);
    clip_poly(gamut_rec2020_rb, cnt, white, v);
    fprintf(stdout, "%g %g\n%g %g\n", v[0], v[1], white[0], white[1]);
  }
#endif

}

// clamp to gamut:
static inline void clip_rgb(float *v)
{
  const float gamut_rec709_rb[] = {
    0.88008f, 0.02299f,   // red
    0.04558f, 0.94246f,   // blue
    0.24632f, 0.06584f,   // green
    0.88008f, 0.02299f,   // red (need to have correct winding order)
  };
  const int cnt = sizeof(gamut_rec709_rb) / sizeof(gamut_rec709_rb[0]) / 2 - 1;

  // used this matrix: rec709 -> rec2020
  // 0.62750375, 0.32927542, 0.04330267,
  // 0.06910828, 0.91951917, 0.0113596,
  // 0.01639406, 0.08801128, 0.89538036

  const float white[] = {0.33333f, 0.33333f};
  clip_poly(gamut_rec709_rb, cnt, white, v);

#if 0 // DEBUG
  srand48(666);
  for(int k=0;k<100;k++)
  {
    float v[] = {drand48() * 1.6 - 0.4, -0.2 + 1.4*drand48()};
    fprintf(stdout, "%g %g\n", v[0], v[1]);
    clip_poly(gamut_rec709_rb, cnt, white, v);
    fprintf(stdout, "%g %g\n%g %g\n", v[0], v[1], white[0], white[1]);
  }
#endif
}

// create 2d pattern of points:
// outer ring: [01]^3 in rgb transformed by wb + cmatrix
// inner ring: [1/4 3/4]^3 transformed by the same
static inline void
create_ring(
    const float *wb,          // camera white balance coefs
    const float *M,           // camera to rec2020 matrix
    const float  saturation,  // saturate the inner ring
    const float *white,       // adjust white point
    float       *source,      // generated list of source points
    float       *target)      // generated list of target points
{
  // 6+6+1 points in 2D RBF: outer, inner, white.
  const int N = 13;
  float in[39] = {
    1, 0, 0,  // red
    1, 1, 0,  // yellow
    0, 1, 0,  // green
    0, 1, 1,  // cyan
    0, 0, 1,  // blue
    1, 0, 1,  // magenta
    3, 1, 1,  // dim red
    3, 3, 1,  // dim yellow
    1, 3, 1,  // dim green
    1, 3, 3,  // dim cyan
    1, 1, 3,  // dim blue
    3, 1, 3,  // dim magenta
    1, 1, 1   // white
  };
  float out[39] = {0};
  for(int k=0;k<N;k++)
    for(int j=0;j<3;j++)
      for(int i=0;i<3;i++)
        out[3*k+j] += M[3*j+i] * in[3*k+i] * wb[i];
  for(int k=0;k<N;k++)
  {
    const float b = out[3*k+0] + out[3*k+1] + out[3*k+2];
    for(int i=0;i<3;i++) out[3*k+i] /= b;
    source[2*k+0] = out[3*k+0];
    source[2*k+1] = out[3*k+2];
    target[2*k+0] = out[3*k+0];
    target[2*k+1] = out[3*k+2];
    clip_spectral_locus(target + 2*k);
  }
  for(int k=0;k<6;k++)
  { // saturation:
    if(saturation < 0.5f)
    { // desaturate outer ring, too:
      float s = saturation * 2.0f;
      target[2*k+0] = (1.0f-s)*white[0] + s*target[2*k+0];
      target[2*k+1] = (1.0f-s)*white[1] + s*target[2*k+1];
      target[2*(k+6)+0] = white[0];
      target[2*(k+6)+1] = white[1];
    }
    else
    { // desaturate inner ring only:
      float s = (saturation - 0.5f)*2.0f;
      target[2*(k+6)+0] = (1.0f-s)*white[0] + s*target[2*(k+6)+0];
      target[2*(k+6)+1] = (1.0f-s)*white[1] + s*target[2*(k+6)+1];
    }
  }
  // move white
  // target[2*12+0] = white[0];
  // target[2*12+1] = white[1];
  source[2*12+0] = white[0];
  source[2*12+1] = white[1];
  // TODO: further parametric manipulations?
  // rotate chroma?
  // move individual colours
  // etc
}


// thinplate spline kernel phi(r).
// note that this one is different to the one used in darktable:
// okay, it's 2d to begin with. but also the threshold added to r2 is crucial,
// or else distance 0 and distance 1 will both evaluate to 0, resulting in
// interesting curves that do not pass through the control points. this is true
// at least in my tests with 5 control points, maybe the effect evens out for more
// points which do not happen to reach exactly 0 and 1 as distance.
static inline double kernel(const float *x, const float *y)
{
  const double r2 = 1e-3 +
      (x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]);
  return r2 * logf(r2);
}

static inline void compute_coefficients(
  const int    N,        // number of patches
  const float *source,   // N 2d source coordinates
  const float *target,   // N 2d target coordinates
  float       *coef)     // will write the 2*(N+2) coefficient vector here, assumed 0 inited
{
  /*
      K. Anjyo, J. P. Lewis, and F. Pighin, "Scattered data
      interpolation for computer graphics," ACM SIGGRAPH 2014 Courses
      on - SIGGRAPH ’14, 2014.
      http://dx.doi.org/10.1145/2614028.2615425
      http://scribblethink.org/Courses/ScatteredInterpolation/scatteredinterpcoursenotes.pdf

      construct the system matrix and the vector of function values and
      solve the set of linear equations

      / R   P \  / c \   / f \
      |       |  |   | = |   |
      \ P^t 0 /  \ d /   \ 0 /

      for the coefficient vector (c d)^t.
      we work in 2D xy chromaticity space.
      our P is a 2x2 matrix (no constant part, we want to keep black black)
  */
  switch(N) // N is the number of patches, i.e. we have N+2 constraints in the matrix
  {
  case 0: // identity
    coef[0+2] = coef[4+3] = 1.0;
    break;
  case 1: // one patch: white balance (we could move, but i don't think we want that)
    coef[4*N+0+2] = target[0] / source[0];
    coef[4*N+4+3] = target[1] / source[1];
    break;
  default: // fully generic case, N patches
  {
    // setup linear system of equations
    int N2 = N+2;
    double *A = malloc(sizeof(*A) * N2 * N2);
    double *b = malloc(sizeof(*b) * N2);
    // coefficients from nonlinear radial kernel functions
    for(int j=0;j<N;j++)
      for(int i=j;i<N;i++)
        A[j*N2+i] = A[i*N2+j] = kernel(source + 2*i, source + 2*j);
    // coefficients from constant and linear functions
    for(int i=0;i<N;i++) A[i*N2+N+0] = A[(N+0)*N2+i] = source[2*i+0];
    for(int i=0;i<N;i++) A[i*N2+N+1] = A[(N+1)*N2+i] = source[2*i+1];
    // lower-right zero block
    for(int j=N;j<N2;j++)
      for(int i=N;i<N2;i++)
        A[j*N2+i] = 0;
    // make coefficient matrix triangular
    int *pivot = malloc(sizeof(*pivot)*N2);
    if (gauss_make_triangular(A, pivot, N2))
    {
      // calculate coefficients for x channel
      for(int i=0;i<N; i++) b[i] = target[2*i];
      for(int i=N;i<N2;i++) b[i] = 0;
      gauss_solve_triangular(A, pivot, b, N2);
      for(int i=0;i<N2;i++) coef[4*i+2] = b[i];
      // calculate coefficients for y channel
      for(int i=0;i<N; i++) b[i] = target[2*i+1];
      for(int i=N;i<N2;i++) b[i] = 0;
      gauss_solve_triangular(A, pivot, b, N2);
      for(int i=0;i<N2;i++) coef[4*i+3] = b[i];
    }
    else
    { // yes, really, we should have continued to use the svd/pseudoinverse for exactly such cases.
      // i might bring it back at some point.
      fprintf(stderr, "[colour] fuck, matrix was singular or something!\n");
      fprintf(stderr, "[colour] N=%d\n", N);
      for(int k=0;k<N;k++)
        fprintf(stderr, "[colour] src = %g %g --> %g %g\n",
            source[2*k+0],
            source[2*k+1],
            target[2*k+0],
            target[2*k+1]);
      fprintf(stderr, "[colour] A = \n");
      for(int j=0;j<N2;j++)
      {
        for(int i=0;i<N2;i++)
          fprintf(stderr, "%.2f\t", A[j*N2+i]);
        fprintf(stderr, "\n");
      }
    }
    // fprintf(stderr, "matrix part %g %g %g %g\n",
    //     coef[4*N+2],
    //     coef[4*N+3],
    //     coef[4*(N+1)+2],
    //     coef[4*(N+1)+3]);
    free(pivot);
    free(b);
    free(A);
  }
  }
}


void commit_params(dt_graph_t *graph, dt_module_t *module)
{
  float *f = (float *)module->committed_param;
  uint32_t *i = (uint32_t *)module->committed_param;
  if(module->img_param.whitebalance[0] > 0)
  {
    for(int k=0;k<4;k++)
      f[k] = powf(2.0f, ((float*)module->param)[0]) *
        module->img_param.whitebalance[k];
  }
  else
    for(int k=0;k<4;k++)
      f[k] = powf(2.0f, ((float*)module->param)[0]);
  for(int k=0;k<12;k++) f[4+k] = 0.0f;
  if(module->img_param.cam_to_rec2020[0] > 0.0f)
  { // camera to rec2020 matrix
    // mat3 in glsl is an array of 3 vec4 column vectors:
    for(int j=0;j<3;j++) for(int i=0;i<3;i++)
      f[4+4*j+i] = module->img_param.cam_to_rec2020[3*j+i];
  }
  else
  { // identity
    f[4+0] = f[4+5] = f[4+10] = 1.0f;
  }
  // grab params by name:
  const float *p_wb  = dt_module_param_float(module, dt_module_get_param(module->so, dt_token("wb")));
  const int    p_cnt = dt_module_param_int  (module, dt_module_get_param(module->so, dt_token("cnt")))[0];
  const float *p_src = dt_module_param_float(module, dt_module_get_param(module->so, dt_token("source")));
  const float *p_tgt = dt_module_param_float(module, dt_module_get_param(module->so, dt_token("target")));
#if 1 // DEBUG
  const float xyz_to_rec2020[] = {
     1.7166511880, -0.3556707838, -0.2533662814,
    -0.6666843518,  1.6164812366,  0.0157685458,
     0.0176398574, -0.0427706133,  0.9421031212};
  for(int j=0;j<3;j++) for(int i=0;i<3;i++)
    f[4+4*j+i] = xyz_to_rec2020[3*j+i];
  float wb[] = {1, 1, 1};
  // TODO: bind some param to white and saturations!
  float white[] = //{0.33333, 0.33333};
  { p_wb[0], p_wb[1] };
  float source[26];
  float target[26];
  create_ring(wb, xyz_to_rec2020,
      // 0.0,
      p_wb[2],
      white, source, target);
  // for(int k=0;k<13;k++)
  //   fprintf(stdout, "%g %g %g %g\n",
  //       source[2*k], source[2*k+1],
  //       target[2*k], target[2*k+1]);
  // exit(43);
#endif
  // init i[16]..i[17] (uvec2 num patches + pad)
  const int N = 13;//((uint32_t*)module->param)[1];
  i[16] = N;
  i[18] = i[19] = i[17] = 0;
  // init f[20]..
  memset(f + 20, 0, sizeof(float)*44);
  for(int k=0;k<N;k++)
  { // source points
    f[20 + 4*k + 0] = source[2*k+0];//((float*)module->param + 15)[2*k+0];
    f[20 + 4*k + 1] = source[2*k+1];//((float*)module->param + 15)[2*k+1];
  }
  compute_coefficients(N,
      source, // ((float*)module->param) + 15, // source
      target, // ((float*)module->param) + 55, // target
      f + 20); //coefs
}

int init(dt_module_t *mod)
{
  // wb, matrix, cnt, source, coef
  mod->committed_param_size = sizeof(float)*(4+12+2+40+44);
  return 0;
}

#if 0
// params:
// black and white points (img)
// white balance coeffs (img)
// colour matrix (img)
// user defined post processing:
// - move white point
// - change saturation
// - clip which gamut
// - how much to move points with low saturation

#endif


#if 0
// processing:
// 1) multiply cam wb coeff or d65 wb coeff to cam rgb
// 2) multiply cam rgb -> XYZ matrix
// 3) keep Y channel (or X+Y+Z) for later
// 4) apply 2d -> 2d rbf on xy:
vec2 coef[N+2]; // RBF coefficients
vec2 vert[N+2]; // RBF vertex position (source of vertex pairs)
out += coef[N+0] * in; // 2x2 matrix, the
out += coef[N+1] * in; // polynomial part
for(int i=0;i<N;i++)
  out += coef[i+2] * kernel(in, vert[i]);
#endif