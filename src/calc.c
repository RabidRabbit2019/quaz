#include <stdint.h>


// вычисление квадратного корня
uint64_t columnSqrt( uint64_t arg )
{
	uint64_t result = 0;
	uint64_t currentStep = 0;
	uint64_t currentLeft = 0;
	int i;
	
	//
	for ( i = 62; i >= 0; i -= 2 )
	{
		//
		currentStep = (currentStep << 2) + ((arg >> i) & 3);
		//
		result <<= 1;
		//
		currentLeft = (result << 1) + 1;
		//
		if ( currentLeft <= currentStep )
		{
			currentStep -= currentLeft;
			++result;
		}
	}
	//
	return result;
}


// 32-битное деление
unsigned int columnDiv( unsigned int arg, unsigned int divider )
{
	unsigned int result = 0;
	int i;
	
	//
	for ( i = 0; (divider < arg) && !(divider & 0x80000000); i++ )
	{
		divider <<= 1;
	}
	//
	for ( ; i >= 0; i-- )
	{
		//
		result <<= 1;
		//
		if ( arg >= divider )
		{
			++result;
			arg -= divider;
		}
		//
		divider >>= 1;
	}
	//
	return result;
}
 

static const int cordicAtnTable6_16[] = {
   2949120
 , 1740967
 , 919879
 , 466945
 , 234379
 , 117304
 , 58666
 , 29335
 , 14668
 , 7334
 , 3667
 , 1833
 , 917
 , 458
 , 229
 , 115
 , 57
 , 29
 , 14
 , 7
 , 4
 , 2
 , 1
};


// вычисление arctan(y/x)
// аргументы в формате с фиксированной точкой 16.16
// x должен быть больше, чем y; оба аргумента > 0
int cordicArcTan( int x, int y )
{
	//
	int z = 0;
	int sigma;
	int x1, y1;
	int i;

	//
	for ( i = 0; i < 22; i++ )
	{
		//
		if ( ! y )
			sigma = 0;
		else
			sigma = y < 0 ? 1 : -1;
		//
		x1 = x - ((sigma * y) >> i);
		y1 = y + ((sigma * x) >> i);
		z = z - sigma * cordicAtnTable6_16[i];
		//
		x = x1;
		y = y1;

	}
	//
	return z;
}


static const int bpfCos[] = {
    65457  // 2.8125
  , 65220  // 5.625
	, 64277  // 11.25
	, 60547  // 22.5
	, 46341  // 45
	, 0      // 90
	, -65536 // 180
};

static const int bpfSin[] = {
     -3216
	,  -6423
	, -12784
	, -25079
	, -46340
	, -65536
	, 0
};




//
// формат 16.16
void BPF(int *x, int *y)
	/*Процедура БПФ*/
    /*x,y входные массивы данных*/
	/*размерностью I=1 БПФ, I=-1 ОБПФ*/
{
	int c,s,t1,t2,t3,t4,u1,u2,u3;
	int i,j,p,l,L,M,M1,K, III = 0;

	L = 128;
	M = 64;
	M1 = 127;

	while ( L >= 2 )
	{
		//
		l = L/2;
		u1 = 1 << 16;
		u2 = 0;
		//
		c = bpfCos[III]; s = bpfSin[III];
		//
		++III;
		//
		for ( j = 0; j < l; j++ )
		{
			for ( i = j; i < 128; i += L )
			{
				p = i + l;
				t1 = x[i] + x[p];
				t2 = y[i] + y[p];
				t3 = x[i] - x[p];
				t4 = y[i] - y[p];
				x[p] = ((int)((((int64_t)t3) * u1) >> 16)) - ((int)((((int64_t)t4) * u2) >> 16));
				y[p] = ((int)((((int64_t)t4) * u1) >> 16)) + ((int)((((int64_t)t3) * u2) >> 16));
				x[i] = t1;
				y[i] = t2;
			}
			u3 = ((int)((((int64_t)u1) * c) >> 16)) - ((int)((((int64_t)u2) * s) >> 16));
			u2 = ((int)((((int64_t)u2) * c) >> 16)) + ((int)((((int64_t)u1) * s) >> 16));
			u1 = u3;
		}
		//
		if ( l > 1 )
		{
			for ( i = 0; i < 128; i++ )
			{
				//
				x[i] /= 2;
				y[i] /= 2;
			}
		}
		//
		L /= 2;
	}
	//
	j = 0;
	//
	for ( i = 0; i < M1; i++ )
	{
		if ( i > j )
		{
			t1 = x[j];
			t2 = y[j];
			x[j] = x[i];
			y[j] = y[i];
			x[i] = t1;
			y[i] = t2;
		}
		K = M;
		//
		while( j >= K )
		{
			j -= K;
			K /= 2;
		}
		j += K;
	}
}


//
int full_atn( int x, int y ) {
  if ( 0 == x && 0 == y ) {
    return 0;
  }
  int ax = x < 0 ? 0 - x : x;
  int ay = y < 0 ? 0 - y : y;
  if ( ax > ay ) {
    if ( x > 0 ) {
      return y < 0 ? 360 * 65536 - cordicArcTan( ax, ay ) : cordicArcTan( ax, ay );
    } else {
      return y < 0 ? 180 * 65536 + cordicArcTan( ax, ay ) : 180 * 65536 - cordicArcTan( ax, ay );
    }
  } else {
    if ( y > 0 ) {
      return x < 0 ? 90 * 65536 + cordicArcTan( ay, ax ) : 90 * 65536 - cordicArcTan( ay, ax );
    } else {
      return x < 0 ? 270 * 65536 - cordicArcTan( ay, ax ) : 270 * 65536 + cordicArcTan( ay, ax );
    }
  }
}
