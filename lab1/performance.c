#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ll long long

void test(ll cycle) {

    ll result = 0;
    clock_t start_time, end_time;
    double cpu_time_used;

    start_time = clock();

    for (ll i = 0; i < cycle; i++) {
        result ^= i;
    }

    end_time = clock();


    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Results : %lld.\n", result);
    printf("%lld times xor CPU time : %f second.\n", cycle, cpu_time_used);
}
int main() {

    test(1e8);
    test(5e8);
    test(1e9);
    test(5e9);
    test(1e10);
   
    return 0;
}
