#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include<fstream>

// state of the system

int	NowYear;		// 2019 - 2024
int	NowMonth;		// 0 - 11

float NowPrecip;	// inches of rain per month
float NowTemp;		// temperature this month
float NowHeight;    // grain height in inches
int NowNumDeer;		// number of deer in the current population
int NowNumDinosaur; // number of Dinosaurs in the current population
int indexMonth = 1;

unsigned int seed = 0;  // a thread-private variable

// Interesting Parameters

const float GRAIN_GROWS_PER_MONTH = 8.0;
const float ONE_DEER_EATS_PER_MONTH = 0.5;

const float AVG_PRECIP_PER_MONTH = 6.0;	    // average
const float AMP_PRECIP_PER_MONTH = 6.0;	    // plus or minus
const float RANDOM_PRECIP =	2.0;	        // plus or minus noise

const float AVG_TEMP = 50.0;	            // average
const float AMP_TEMP = 20.0;	            // plus or minus
const float RANDOM_TEMP = 10.0;	            // plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP =	10.0;

float SQR( float x )
{
        return x*x;
}

float Ranf( unsigned int *seedp,  float low, float high )
{
        float r = (float) rand_r( seedp );              // 0 - RAND_MAX

        return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}

void Grain()
{
  while( NowYear < 2025 )
    {
        int tempHeight = NowHeight;
	    // compute a temporary next-value for this quantity
	    // based on the current state of the simulation:
        float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
        float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );


        tempHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        tempHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;

        if (tempHeight < 0.)
        {
            tempHeight = 0.;
        }
	    // DoneComputing barrier:
	    #pragma omp barrier
	    NowHeight = tempHeight;

	    // DoneAssigning barrier:
	    #pragma omp barrier
	
	    // DonePrinting barrier:
	    #pragma omp barrier	
    }  
}

void GrainDeer()
{
    while( NowYear < 2025 )
    {
        int tempdeer = NowNumDeer;
        if(tempdeer > NowHeight)
        {
            tempdeer--;
            if(tempdeer < 0)
                tempdeer = 0;
        }
        else
        {
            tempdeer++;
        }
        // DoneComputing barrier:
	    #pragma omp barrier
	    NowNumDeer = tempdeer;

	    // DoneAssigning barrier:
	    #pragma omp barrier
	
	    // DonePrinting barrier:
	    #pragma omp barrier	
    }
}

void MyDinosaur()
{
    while( NowYear < 2025 )
    {
        int tempDino = NowNumDinosaur;
        if(tempDino >= (NowNumDeer/2))
        {
            tempDino = tempDino - 1;
            if(tempDino < 0)
                tempDino = 0;
        }
        else if( tempDino < (NowNumDeer/4))
        {
            tempDino = tempDino + 1;
        }
        // DoneComputing barrier:
	    #pragma omp barrier
	    NowNumDinosaur = tempDino;

	    // DoneAssigning barrier:
	    #pragma omp barrier
	
	    // DonePrinting barrier:
	    #pragma omp barrier	
    }

}

void Watcher( )
{
    while( NowYear < 2025 )
    {
        // DoneComputing barrier:
	    #pragma omp barrier

	    // DoneAssigning barrier:
	    #pragma omp barrier

        float New_Precip = NowPrecip * 2.54;
        float New_Temp = (5./9.)*(NowTemp-32);

        printf(" %d     %d      %8.2f       %8.2f       %d      %8.2f       %d\n", NowYear, indexMonth, New_Temp, New_Precip, NowNumDeer, NowHeight, NowNumDinosaur);
        std::ofstream file;
	    file.open("output3.txt", std::ios_base::app);
	    file << NowYear << " "
		 << indexMonth << " "
		 << New_Temp << " "
		 << New_Precip<< " "
         << NowNumDeer<< " "
         << NowHeight<< " "
         << NowNumDinosaur<< "\n";
        file.close();

        indexMonth++;

        NowMonth++;
        if(NowMonth > 11)
        {
            NowYear = NowYear + 1;
            NowMonth = 0;
        }

        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. )
	        NowPrecip = 0.;
        // DonePrinting barrier:
	    #pragma omp barrier	
    }

}



int main( int argc, char *argv[ ] )
{
   #ifndef _OPENMP
	fprintf( stderr, "No OpenMP support!\n" );
	return 1;
	#endif

    // starting date and time:
    NowMonth =    0;
    NowYear  = 2019;

    // starting state (feel free to change this if you want):
    NowNumDeer = 1;
    NowHeight =  1.; 
    NowNumDinosaur = 1;

    float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );

    float temp = AVG_TEMP - AMP_TEMP * cos( ang );
    unsigned int seed = 0;
    NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
    NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
    if( NowPrecip < 0. )
	    NowPrecip = 0.;

    omp_set_num_threads( 4 );	// same as # of sections
    #pragma omp parallel sections
    {
	    #pragma omp section
	    {
		    GrainDeer( );
	    }

	    #pragma omp section
	    {
		    Grain( );
	    }

	    #pragma omp section
	    {
		    Watcher( );
	    }

	    #pragma omp section
	    {
		    MyDinosaur( );	// your agent
	    }
    }        // implied barrier -- all functions must return in order
	        // to allow any of them to get past here
    return 0;
}