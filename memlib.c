// memlib : 메모리 시스템 모델 (목적 → 설계할 할당기가 기존의 시스템 수준의 malloc 패키지와 상관 없이 돌 수 있도록 하기 위함)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "memlib.h"
#include "config.h"

/* 변수 참고 */
// mem_start_brk ~ mem_brk 사이의 바이트들 → 할당 가상 메모리 (allocated)
// mem_brk ~ (mem_max_addr) 사이의 바이트들 → 미할당 가상 메모리 (free)

static char *mem_start_brk;  /* 힙의 첫 바이트를 가리키는 변수 */
static char *mem_brk;        /* 힙의 마지막 바이트를 가리키는 변수 */
static char *mem_max_addr;   /* 힙의 최대 크기(20MB) 주소를 가리키는 변수 (→ 이 이상으로는 할당할 수 없다.)*/ 

void mem_init(void);         
void mem_deinit(void);       
void mem_reset_brk();        
void* mem_sbrk(int incr);    
void *mem_heap_lo();
void *mem_heap_hi();
size_t mem_heapsize(); 
size_t mem_pagesize();

/* 1. 할당기 초기화
    : 힙에 가용한 가상메모리를 큰 더블 워드로 정렬된 바이트의 배열로 모델화 한다. (성공 0, 실패 -1) */ 
void mem_init(void)
{   
    // 1) 20MB(MAX_HEAP) 만큼의 메모리를 malloc으로 동적할당 하여 mem_start_brk에 넣는다. (실패할 경우, 에러메시지 + 종료) 
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }

    // 2) 아직 힙(가상 메모리)는 비어 있으므로, 각 포인터 변수를 아래와 같이 초기화 한다. (시작 = 끝)
    mem_max_addr = mem_start_brk + MAX_HEAP;  
    mem_brk = mem_start_brk;                  
}

/* 2. (할당기를 통해) 힙(가용 리스트) 메모리를 반환한다. → 힙 자체 삭제 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/* 3. (할당기를 통해) 빈 힙을 표현하기 위해, 할당기의 포인터를 리셋한다. */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 4. 추가적인 힙 메모리 요청하는 함수 (힙 축소 요청은 거부)
    : byte 단위로 필요한 메모리의 크기를 입력 받아, 그 크기만큼 힙을 늘려주고 새 메모리의 시작 지점을 리턴 */
void *mem_sbrk(int incr) // incr -> 바이트 크기
{   
    // 1) 힙을 늘리기 전, 끝 포인터를 임시 변수에 저장한다.
    char *old_brk = mem_brk;

    // 2) 힙이 줄어들거나 최대 힙 사이즈를 벗어난다면 → 메모리 부족으로 처리하고 -1을 리턴한다.
    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n"); // "ERROR:~" 에러 메시지를 stderr 파일 스트림을 통해 출력한다.
	return (void *)-1;  // ※ 리턴형이 void*이므로, 이에 맞춰 형변환 한다.
    }

    // incr(증가된 범위)를 mem_brk에 업데이트 해주고, 새로 늘어난 힙의 첫 번째 주소를 리턴해준다. 
    mem_brk += incr;
    return (void *)old_brk;
}

/* 5. first heap byte 주소를 리턴한다. */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 6. last heap byte 주소를 리턴한다. */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/* 7. 힙 사이즈를 리턴한다. */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/* 8. 페이지 사이즈를 리턴한다. */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}

// ※ (8)페이지 사이즈는 운영체제 수준에서 메모리 관리 단위로, (7)힙 사이즈는 프로그램 내에서 동적으로 할당된 메모리 블록들의 크기를 나타내는 값이다.
