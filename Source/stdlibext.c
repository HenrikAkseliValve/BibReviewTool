#include <stdint.h>
#include "stdlibext.h"

uint32_t i32toalen(int32_t integer){
	if(integer==0) return 1;
	else{
		uint32_t len;
		register int32_t temp=integer;
		for(len=0;temp!=0;len++,temp/=10);
		return len;
	}
}

void i32toa(const int32_t integer,uint8_t *buffer,const uint32_t len){
	if(integer==0) buffer[0]='0';
	else{
		int32_t i=len-1;
		register int32_t temp=integer;
		while(temp!=0){
			int32_t rem=temp%10;
			buffer[i--]=rem+'0';
			temp=temp/10;
		}
	}
}

uint32_t i64toalen(int64_t integer){
	if(integer==0) return 1;
	else{
		uint32_t len;
		register int32_t temp=integer;
		for(len=0;temp!=0;len++,temp/=10);
		return len;
	}
}

void i64toa(const int64_t integer,uint8_t *buffer,const uint32_t len){
	if(integer==0) buffer[0]='0';
	else{
		int64_t i=len-1;
		register int64_t temp=integer;
		while(temp!=0){
			int32_t rem=temp%10;
			buffer[i--]=rem+'0';
			temp=temp/10;
		}
	}
}
