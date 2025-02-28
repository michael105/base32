#if 0
COMPILE errno
#SHRINKELF
STRIPFLAG
OPTFLAG -O3

return
#endif

/* 
 base32 encoder
 encodes from stdin to stdout
 uses the (standard) encoding A-Z 2-8
 There's one optional argument, the position of the linebreak.
 e.g. base32 80
 default is 76
 0 means no linebreaks at all

 It's a branchless approach, 
 and uses the instructions bswap, ror and rol. 
 (might be present at most systems)

 (c)2022, Michael (misc) Myer, GPL

*/

#ifndef MLIB
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#endif

// the syscalls slow down.
// would be possible to spare several writes,
// by inserting the linebreaks into the outputbuffer,
// and enlarge the buffer.
// slows down from 150MB/s to 30 MB/s
// but. hey.
// enlarging BUF and set wrap to 0 (disable)
// would also spare syscalls.
// (I'm a bit buffled, that the write syscalls are that expensive..)

// BUF has to be a multiple of 5
#define BUF 4000


// Byteswap
#define BSWAP(a) asm("bswap %0" : "+r"(a) )

// bitrotation 
#define ROL(bits,a) asm("rol $"#bits",%0" : "+r"(a) )
#define ROR(bits,a) asm("ror $"#bits",%0" : "+r"(a) )


// read with repetition on eintr,..,  needed especially for reading pipes,
// when the buffer runs empty
int nread( int fd, char *buf, int len ){
	char *b = buf;
	char *e = buf+len;

	do {
		errno = 0;
		int ret = read(fd,b,(e-b));
		if ( ret <= 0 ){
			if ( errno == EAGAIN || errno == ENOMEM || errno == EINTR )
				continue;
			return( b-buf ); // rw bytes (if)
		}
		b+=ret;
	} while ( b < e );

	return( len );
}

// returns a pointer to the last written char.
// no 0 is appended.
// if len modulo 5 is not 0,
// == is appended
// size of buf must be divisible by 5, filled up with 0s, if (len % 5)
// 0: 130 MB/s
// rnd: 151 MB/s
// -O3 180MB/s
static char* base32enc(char *obuf, const char* buf, uint len){
	const char* e = buf+len;
	char *pobuf = obuf;

	while ( buf < e ){
		long l = *(long*)buf;
		long lo = 0;
		BSWAP(l);

		char *pobuf_t = pobuf;
		do {
			pobuf++;
			ROL(5,l);
			char c = (l&0x1f);
			// convert to A-Z / 2-8 
			c += 24 + (((c-26)>>8) & 41 );
			lo |= c;
			ROR(8,lo);
		} while ( pobuf - 8 < pobuf_t );
		//} while ( pobuf - pobuf_t < 8 );

		*(long*)pobuf_t = lo;
		buf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( buf > e ){
		len<<=3;
		const int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			len+=5;
		}
	}

	return(pobuf);
}

// 0: 153MB/s
// rnd: 153MB/s
//  -> 158 MB/s
//  -O3 174 MB/s
char* base32encB(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;
	unsigned long l;

	while ( pbuf < buf+len ){
		l = *(long*)pbuf;
		long lo = 0;
		BSWAP(l);
		//for ( int a = 0; a<8; a++ ){
			for ( int a = 8; a-->0; ){
			//char c =(l >> 59) + 65;
			ROL(5,l);
			//char c = (l&0x1f) + 65;
			//if ( c > 90 ) 
			//	c-=41;

			char c = (l&0x1f);
			// convert to A-Z / 2-8 
			c += 24 + (((c-26)>>8) & 41 );

			//lo = (lo>>8) | ((long)c<<56);
			lo |= c;
			ROR(8,lo);
			//*pobuf = c;
			//pobuf++;

			//l <<= 5;
		}
		*(long*)pobuf = lo;
		pobuf += 8;

		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}

// 0: 192MB/s
// rnd: 90MB/s
// -O3 165 MB/s
char* base32encC(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;
	unsigned long l;

	while ( pbuf < buf+len ){
		l = *(long*)pbuf;
		long lo = 0;
		BSWAP(l);
		for ( int a = 0; a<8; a++ ){
			//for ( int a = 8; a-->0; ){
			//char c =(l >> 59) + 65;
			ROL(5,l);
			char c = (l&0x1f) + 65;
			if ( c > 90 ) 
				c-=41;

			//char c = (l&0x1f);
			// convert to A-Z / 2-8 
			//c += 24 + (((c-26)>>8) & 41 );

			//lo = (lo>>8) | ((long)c<<56);
			lo |= c;
			ROR(8,lo);
			//*pobuf = c;
			//pobuf++;

			//l <<= 5;
		}
		*(long*)pobuf = lo;
		pobuf += 8;

		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}

// 0: 198 MB/s
// rnd: 90.8MB/s
// -O3 216 MB/s
char* base32encD(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;

	while ( pbuf < buf+len ){
		long l = *(long*)pbuf;
		BSWAP(l);
		for ( int a = 0; a<8; a++ ){
			ROL(5,l);
			char c = (l&0x1f) + 65;
			if ( c > 90 ) 
				c-=41;
			*pobuf = c;
			pobuf++;
		}
		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}

//rnd: 184 MB/s
// -O3 216 MB/s
char* base32encE(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;

	while ( pbuf < buf+len ){
		long l = *(long*)pbuf;
		BSWAP(l);
		//for ( int a = 0; a<8; a++ ){
		for ( int a = 8; a-->0; ){
			ROL(5,l);
			char c = (l&0x1f);
			// convert to A-Z / 2-8 
			c += 24 + (((c-26)>>8) & 41 );
			*pobuf = c;
			pobuf++;
		}
		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}

// rnd: 194 MB/s
// -> rol -> 198 MB/s
// -O3 208MB/s
char* base32encD2(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;

	while ( pbuf < buf+len ){
		long l = *(long*)pbuf;
		BSWAP(l);
			ROL(5,l);
		//for ( int a = 0; a<8; a++ ){
		//asm("mark:");
		for ( int a = 8; a-->0; ){
			char c = (l&0x1f);
			ROL(5,l);
			// convert to A-Z / 2-8 
			if ( c < 26 )
				c+=65;
			else
				c+=24;

			*pobuf = c;
			pobuf++;
		}
		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}

// rnd: 180 MB/s
// -O3: 218MB/s
char* base32encE4(char *obuf, const char* buf, uint len){
	const	char *pbuf = buf;
	char *pobuf = obuf;
	unsigned long l;

	while ( pbuf < buf+len ){
		l = *(long*)pbuf;
		long lo = 0;
		BSWAP(l);
		//for ( int a = 0; a<8; a++ ){
		asm("mark:");
		for ( int a = 8; a-->0; ){
			ROL(5,l);
			char c = (l&0x1f);
			char c2 = c + 24;
			c2 += ((c-26)>>8)&41;
			// convert to A-Z / 2-8 
			//if ( c > 25 )
			//	c+=24;
			//else
			//	c+=65;
/*			asm volatile ("mov %1,%0\n"
				"add $24,%0\n"
				"sub $26,%1\n"
				"shr $8,%1\n"
				"and $41,%1\n"
				"add %1,%0\n" : "+r"(c2) : "r"(c) ); */


			*pobuf = c2;
			pobuf++;
		}
		pbuf += 5;
	}

	// fill end of a not fully filled buffer with ==
	if ( pbuf > buf + len ){
		//r*=8;
		len<<=3;
		int i = (pobuf-obuf-1) * 5;
		char *p = pobuf;
		while ( i > len ){
			*(--p) = '=';
			i-=5;
		}
	}

	return(pobuf);
}




// linux utils, base32 with linewrap off is 178 MB/s
// ; however - it comes with 47kB - dynamically linked, to uClibc.
// opposed to 1kB for this, statically linked



#define writes(s) write(1,s,sizeof(s));
void usage(){
	writes("usage: input | base32 [pos of linebreak]\n"
	"GPL, (C)2022 Michael (misc) Myer\n"
	"github.com/michael105\n");

	exit(1);
}


int main(int argc, char **argv){
	char buf[BUF+8];
	char obuf[BUF*8/5+64];

	int r;
	int wrap = 76; // wrap
	int wrote = 0;

	if ( argc > 1 ){
		wrap = 0;
		for ( char *p = argv[1]; *p; p++ ){
			if ( *p < '0' || *p > '9' )
				usage();
			wrap = wrap*10 + *p-'0';
		}
	}

	while( (r=nread(0,buf,BUF)) > 0 ) {

		if ( r < BUF )
			*(long*)(buf+r) = (long)0;

		char *pobuf = base32encD(obuf,buf,r);
			
		// wrapping and output
		char *ob = obuf;
		if ( wrap ){
			while ( (pobuf-ob) >= wrap-wrote ){
				write(1,ob,wrap-wrote);
				write(1,"\n",1);
				ob += wrap-wrote;
				wrote = 0;
			}
		}
		wrote = write(1,ob,(pobuf-ob));
	};

	if ( wrote )
		write(1,"\n",1);

	exit(0);
}
