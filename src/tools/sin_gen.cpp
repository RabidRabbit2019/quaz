#include <math.h>
#include <stdio.h>
#include <stdint.h>


#define PI  3.14159265358


int main( int argc, char ** argv) {
  if ( 4 != argc ) {
    printf( "usage: %s <samples_count> <samples_min> <samples_max>\n", argv[0] );
    return 1;
  }
  
  size_t v_count = ::strtoul( argv[1], 0, 10 );
  if ( 0 == v_count ) {
    return 1;
  }


  long int v_min = ::strtol( argv[2], 0, 10 );
  long int v_max = ::strtol( argv[3], 0, 10 );
  
  if ( v_min >= v_max ) {
    return 1;
  }
  
  double v_amp = ((v_max - v_min) / 2.0) - 0.000001;
  double v_zero = v_min + v_amp;
  double v_step = (PI * 360.0 / v_count) / 180.0;
  ::printf( "static const int g_cos_table[%lu] = {\n", v_count );
  double v_ang = 0;
  for ( size_t i = 0; i < v_count; ++i ) {
    if ( 0 != i && 0 == (i % 8) ) {
      ::printf( "\n" );
    }
    if ( 0 == i ) {
      ::printf( "  " );
    } else {
      ::printf( ", " );
    }
    ::printf( "%d", (int)(v_zero + v_amp * ::sin( v_ang )) );
    v_ang += v_step;
  }
  ::printf( "\n};\n" );

  return 0;
}
