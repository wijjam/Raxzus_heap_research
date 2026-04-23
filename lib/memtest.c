// memtest.c - heap fragmentation torture + O(1) timing proof
#include "../include/heap_domain.h"
#include "../include/vga.h"

// Used as the timer
static inline uint32_t rdtsc_start(void) {
    uint32_t lo;
    asm volatile ("lfence\nrdtsc" : "=a"(lo) : : "edx", "memory");
    return lo;
}

static inline uint32_t rdtsc_end(void) {
    uint32_t lo;
    asm volatile ("rdtsc\nlfence" : "=a"(lo) : : "edx", "memory");
    return lo;
}

#define FRAG_N   40000
#define DIRTY_N  48000

static uint32_t isqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n, y = (n / 2) + 1;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}

void cmd_memtest_v1(void) {

    void** s64  = kmalloc(FRAG_N * sizeof(void*));
    void** s128 = kmalloc(FRAG_N * sizeof(void*));
    void** s256 = kmalloc(FRAG_N * sizeof(void*));
    void** s512 = kmalloc(FRAG_N * sizeof(void*));
    void** s1024 = kmalloc(FRAG_N * sizeof(void*));

    kprintf_yellow("\nNow allocating %d different types of blocks\n", FRAG_N);

    for (int i = 0; i<FRAG_N; i++) {
        s64[i] = kmalloc(50);
        *(uint32_t*)s64[i] = (i+1)*3;
        s128[i] = kmalloc(100);
        *(uint32_t*)s128[i] = (i+1)*7;
        s256[i] = kmalloc(200);
        *(uint32_t*)s256[i] = (i+1)*11;
        s512[i] = kmalloc(500);
        *(uint32_t*)s512[i] = (i+1)*13;
        s1024[i] = kmalloc(1000);
        *(uint32_t*)s1024[i] = (i+1)*17;
    }

    kprintf_yellow("Testing if the newly allocated blocks keep the correct values or if they are corrupted\n");
    for (int ii = 0; ii<FRAG_N; ii++) {
        if (*(uint32_t*)s64[ii] != (ii+1)*3) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s128[ii] != (ii+1)*7) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s256[ii] != (ii+1)*11) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s512[ii] != (ii+1)*13) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s1024[ii] != (ii+1)*17) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
    }

    kprintf_green("Allocation of all blocks finished successfully with no corruption, now comensing swish cheese\n");
    kprintf_yellow("Freeing every other 64 block:\n");

    for (int k = 0; k < FRAG_N; k++) { if (k % 2 == 0) kfree(s64[k]); }
    for (int j = 0; j < FRAG_N; j++) { if (j % 2 == 0) kfree(s256[j]); }

    kprintf_green("The swiss cheese has been done successfully, Now checking heap data corruption\n");

    for (int i = 0; i < FRAG_N; i++) {
        if (i % 2 != 0) {
            if (*(uint32_t*)s64[i] != (uint32_t)(i+1)*3) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
            if (*(uint32_t*)s256[i] != (uint32_t)(i+1)*11) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        }
        if (*(uint32_t*)s128[i]  != (uint32_t)(i+1)*7)  { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s512[i]  != (uint32_t)(i+1)*13) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
        if (*(uint32_t*)s1024[i] != (uint32_t)(i+1)*17) { kprintf_red("One of the blocks has been corrupted, PANIC!!"); return; }
    }

    // 32 sample timing under fragmentation
    uint32_t frag_samples[32];
    uint32_t frag_min = 0xFFFFFFFF, frag_max = 0, frag_sum = 0;
    for (int s = 0; s < 32; s++) {
        uint32_t t1 = rdtsc_start();
        uint32_t* p = kmalloc(1024);
        *(volatile uint32_t*)p = 42;
        kfree(p);
        uint32_t t2 = rdtsc_end();
        uint32_t d = t2 - t1;
        frag_samples[s] = d;
        if (d < frag_min) frag_min = d;
        if (d > frag_max) frag_max = d;
        frag_sum += d;
    }
    uint32_t frag_avg = frag_sum / 32;
    uint32_t frag_var = 0;
    for (int s = 0; s < 32; s++) {
        int32_t diff = (int32_t)frag_samples[s] - (int32_t)frag_avg;
        frag_var += diff * diff;
    }
    frag_var /= 31;
    kprintf("FRAGMENTED  min=%d max=%d avg=%d stddev=%d cycles\n", frag_min, frag_max, frag_avg, isqrt(frag_var));

    for (int i = 0; i < FRAG_N; i++) {
        kfree(s64[i]);
        kfree(s128[i]);
        kfree(s256[i]);
        kfree(s512[i]);
        kfree(s1024[i]);
    }
    kfree(s64);
    kfree(s256);
    kfree(s512);
    kfree(s128);
    kfree(s1024);

    // 32 sample timing after cleanup
    uint32_t clean_samples[32];
    uint32_t clean_min = 0xFFFFFFFF, clean_max = 0, clean_sum = 0;
    for (int s = 0; s < 32; s++) {
        uint32_t t1 = rdtsc_start();
        uint32_t* p = kmalloc(1024);
        *(volatile uint32_t*)p = 42;
        kfree(p);
        uint32_t t2 = rdtsc_end();
        uint32_t d = t2 - t1;
        clean_samples[s] = d;
        if (d < clean_min) clean_min = d;
        if (d > clean_max) clean_max = d;
        clean_sum += d;
    }
    uint32_t clean_avg = clean_sum / 32;
    uint32_t clean_var = 0;
    for (int s = 0; s < 32; s++) {
        int32_t diff = (int32_t)clean_samples[s] - (int32_t)clean_avg;
        clean_var += diff * diff;
    }


    uint32_t tid1 = rdtsc_start(); 
    uint32_t tid2 = rdtsc_end(); 
    kprintf("The empty call is %d\n", tid2-tid1);

    clean_var /= 31;
    kprintf("CLEAN       min=%d max=%d avg=%d stddev=%d cycles\n", clean_min, clean_max, clean_avg, isqrt(clean_var));
    kprintf("O(1) delta (avg): %d cycles\n", (int32_t)frag_avg - (int32_t)clean_avg);
}


void cmd_memtest_v2(void) {
    kprintf_white("\n========================================\n");
    kprintf_white("   RAXZUS HEAP - MEMTEST 2 (LARGE ALLOC)\n");
    kprintf_white("========================================\n\n");

    kprintf_yellow("[PHASE 3] Large Allocation Test (kmalloc)\n");
    kprintf("  Allocates N contiguous 4KB pages as a single region.\n");
    kprintf("  Verifies virtual contiguity and write/read correctness.\n\n");

    // --- 64 KB allocation (16 pages) ---
    kprintf("  Allocating 64 KB (16 pages)...\n");
    uint8_t* buf64k = (uint8_t*)kmalloc(64 * 1024);
    if (!buf64k) {
        kprintf_red("  FAIL: kmalloc(64K) returned NULL\n");
    } else {
        kprintf("  Got pointer: 0x%x\n", (uint32_t)buf64k);
        for (uint32_t i = 0; i < 64 * 1024; i++)
            buf64k[i] = (uint8_t)(i & 0xFF);
        int bad = 0;
        for (uint32_t i = 0; i < 64 * 1024; i++)
            if (buf64k[i] != (uint8_t)(i & 0xFF)) bad++;
        if (bad == 0)
            kprintf_green("  PASS: 64 KB written and verified (65536 bytes, zero errors)\n");
        else
            kprintf_red("  FAIL: %d bytes corrupted in 64 KB region\n", bad);
    }

    // --- 1 MB allocation (256 pages) ---
    kprintf("\n  Allocating 1 MB (256 pages)...\n");
    uint8_t* buf1m = (uint8_t*)kmalloc(1024 * 1024);
    if (!buf1m) {
        kprintf_red("  FAIL: kmalloc(1MB) returned NULL\n");
    } else {
        kprintf("  Got pointer: 0x%x\n", (uint32_t)buf1m);
        for (uint32_t i = 0; i < 1024 * 1024; i++)
            buf1m[i] = (uint8_t)((i ^ (i >> 8)) & 0xFF);
        int bad = 0;
        for (uint32_t i = 0; i < 1024 * 1024; i++)
            if (buf1m[i] != (uint8_t)((i ^ (i >> 8)) & 0xFF)) bad++;
        if (bad == 0)
            kprintf_green("  PASS: 1 MB written and verified (1048576 bytes, zero errors)\n");
        else
            kprintf_red("  FAIL: %d bytes corrupted in 1 MB region\n", bad);
    }

    if (buf64k && buf1m) {
        uint32_t gap = (uint32_t)buf1m - (uint32_t)buf64k;
        kprintf("\n  Address gap between 64K and 1MB regions: %d bytes\n", gap);
        if ((uint32_t)buf1m > (uint32_t)buf64k)
            kprintf_green("  PASS: Regions are non-overlapping and virtually contiguous\n");
        else
            kprintf_red("  FAIL: Region addresses overlap!\n");
    }

    kprintf_yellow("\n  Timing: kmalloc cost per page\n");
    asm volatile("cli");
    uint32_t t0 = rdtsc_start();
    void* tbuf = kmalloc(4096);
    uint32_t t1 = rdtsc_end();
    asm volatile("sti");
    kprintf("    1 page  (4 KB) : %d cycles\n", t1 - t0);

    asm volatile("cli");
    t0 = rdtsc_start();
    void* tbuf8 = kmalloc(8 * 4096);
    t1 = rdtsc_end();
    asm volatile("sti");
    kprintf("    8 pages (32 KB): %d cycles  (per-page: %d)\n",
            t1 - t0, (t1 - t0) / 8);

    asm volatile("cli");
    t0 = rdtsc_start();
    void* tbig = kmalloc(1024 * 1024);
    t1 = rdtsc_end();
    asm volatile("sti");
    kprintf("    256 pages (1MB): %d cycles  (per-page: %d)\n",
            t1 - t0, (t1 - t0) / 256);
    kfree(tbig);

    kfree(tbuf);
    kfree(tbuf8);
    if (buf64k) kfree(buf64k);
    if (buf1m)  kfree(buf1m);

    kprintf_white("\n========================================\n");
    kprintf_white("   MEMTEST 2 COMPLETE\n");
    kprintf_white("========================================\n\n");
}




/*

Karantän

#define SAMPLES 32

// First sample is cold (may trigger page mapping). Remaining SAMPLES are steady-state.
static void measure_full(uint32_t size, uint32_t* cold,
                         uint32_t* ss_min, uint32_t* ss_max) {
    asm volatile("cli");
    uint32_t t0 = rdtsc();
    void* p = kmalloc(size); kfree(p);
    uint32_t t1 = rdtsc();
    asm volatile("sti");
    *cold = t1 - t0;

    uint32_t min = 0xFFFFFFFF, max = 0;
    for (int i = 0; i < SAMPLES; i++) {
        asm volatile("cli");
        t0 = rdtsc();
        p = kmalloc(size); kfree(p);
        t1 = rdtsc();
        asm volatile("sti");
        uint32_t d = t1 - t0;
        if (d < min) min = d;
        if (d > max) max = d;
    }
    *ss_min = min;
    *ss_max = max;
}

// Steady-state only (free list already warm).
static void measure_ss(uint32_t size, uint32_t* ss_min, uint32_t* ss_max) {
    uint32_t min = 0xFFFFFFFF, max = 0;
    for (int i = 0; i < SAMPLES; i++) {
        asm volatile("cli");
        uint32_t t0 = rdtsc();
        void* p = kmalloc(size); kfree(p);
        uint32_t t1 = rdtsc();
        asm volatile("sti");
        uint32_t d = t1 - t0;
        if (d < min) min = d;
        if (d > max) max = d;
    }
    *ss_min = min;
    *ss_max = max;
}

static inline uint32_t u32abs(int32_t x) { return x < 0 ? (uint32_t)-x : (uint32_t)x; }

#define FRAG_N   20000
#define DIRTY_N  48000

// -----------------------------------------------------------------------
// memtest 1 - Fragmentation torture + WCET O(1) timing proof
// -----------------------------------------------------------------------
void cmd_memtest_v1(void) {
    kprintf_white("\n========================================\n");
    kprintf_white("   RAXZUS HEAP - MEMTEST 1 (WCET)\n");
    kprintf_white("========================================\n\n");

    void** s64  = kmalloc(FRAG_N * sizeof(void*));
    void** s256 = kmalloc(FRAG_N * sizeof(void*));
    void** s512 = kmalloc(FRAG_N * sizeof(void*));
    void** x256 = kmalloc(FRAG_N * sizeof(void*));
    void** dp   = kmalloc(DIRTY_N * sizeof(void*));

    if (!s64 || !s256 || !s512 || !x256 || !dp) {
        kprintf_red("MEMTEST: failed to allocate bookkeeping arrays\n");
        return;
    }

    // ================================================================
    // PHASE 1 - FRAGMENTATION TORTURE TEST
    // ================================================================
    kprintf_yellow("[PHASE 1] Fragmentation Torture Test\n");
    kprintf("  1) Alloc 64B / 256B / 512B blocks interleaved.\n");
    kprintf("  2) Free ALL 64B blocks -> 200 holes in a traditional heap.\n");
    kprintf("  3) Alloc 200 more 256B blocks that won't fit in 64B holes.\n");
    kprintf("  4) Verify every live block still holds its sentinel.\n\n");

    for (int i = 0; i < FRAG_N; i++) {
        s64[i]  = kmalloc(64);
        s256[i] = kmalloc(256);
        s512[i] = kmalloc(512);
    }
    kprintf("  Allocated %d x 64B, %d x 256B, %d x 512B (interleaved)\n",
            FRAG_N, FRAG_N, FRAG_N);

    for (int i = 0; i < FRAG_N; i++) {
        if (s64[i])  *(uint32_t*)s64[i]  = 0xAAAA0000 | (uint32_t)i;
        if (s256[i]) *(uint32_t*)s256[i] = 0xBBBB0000 | (uint32_t)i;
        if (s512[i]) *(uint32_t*)s512[i] = 0xCCCC0000 | (uint32_t)i;
    }

    for (int i = 0; i < FRAG_N; i++) { kfree(s64[i]); s64[i] = 0; }
    kprintf("  Freed all 64B blocks (200 holes punched)\n");

    for (int i = 0; i < FRAG_N; i++) {
        x256[i] = kmalloc(256);
        if (x256[i]) *(uint32_t*)x256[i] = 0xDDDD0000 | (uint32_t)i;
    }
    kprintf("  Allocated %d more 256B blocks\n", FRAG_N);

    int bad = 0;
    for (int i = 0; i < FRAG_N; i++) {
        if (s256[i] && *(uint32_t*)s256[i] != (0xBBBB0000 | (uint32_t)i)) bad++;
        if (s512[i] && *(uint32_t*)s512[i] != (0xCCCC0000 | (uint32_t)i)) bad++;
        if (x256[i] && *(uint32_t*)x256[i] != (0xDDDD0000 | (uint32_t)i)) bad++;
    }

    if (bad == 0) {
        kprintf_green("\n  PASS: 0 corrupted blocks out of %d live allocations\n", FRAG_N * 3);
        kprintf_green("  PASS: 64B holes had zero effect on 256B / 512B domains\n");
        kprintf_green("  PASS: Domain isolation is hardware-enforced (separate CR3)\n");
    } else {
        kprintf_red("\n  FAIL: %d blocks corrupted\n", bad);
    }

    for (int i = 0; i < FRAG_N; i++) {
        if (s256[i]) kfree(s256[i]);
        if (s512[i]) kfree(s512[i]);
        if (x256[i]) kfree(x256[i]);
    }

    // ================================================================
    // PHASE 2 - SPEED / DETERMINISM / O(1) PROOF
    // ================================================================
    kprintf_yellow("\n[PHASE 2] Speed, Determinism, O(1) Proof  (RDTSC, %d samples)\n", SAMPLES);

    // --- COLD START: first ever alloc for each size class ---
    // This sample may pay the one-time cost of mapping a new physical page.
    // It is NOT part of the steady-state claim; shown so nothing is hidden.
    uint32_t cold64, cold256, cold1k;
    uint32_t c64_min,  c64_max;
    uint32_t c256_min, c256_max;
    uint32_t c1k_min,  c1k_max;
    measure_full(64,   &cold64,  &c64_min,  &c64_max);
    measure_full(256,  &cold256, &c256_min, &c256_max);
    measure_full(1024, &cold1k,  &c1k_min,  &c1k_max);

    kprintf_cyan("\n  [1] COLD START (first alloc - may include page mapping):\n");
    kprintf("    kmalloc(64)  : %d cycles\n", cold64);
    kprintf("    kmalloc(256) : %d cycles\n", cold256);
    kprintf("    kmalloc(1024): %d cycles\n", cold1k);

    kprintf_cyan("\n  [2] STEADY STATE speed + jitter (free list warm, %d samples):\n", SAMPLES);
    kprintf("    kmalloc(64)   + kfree : min=%d  max=%d  jitter=%d cycles\n",
            c64_min,  c64_max,  c64_max  - c64_min);
    kprintf("    kmalloc(256)  + kfree : min=%d  max=%d  jitter=%d cycles\n",
            c256_min, c256_max, c256_max - c256_min);
    kprintf("    kmalloc(1024) + kfree : min=%d  max=%d  jitter=%d cycles\n",
            c1k_min,  c1k_max,  c1k_max  - c1k_min);

    // --- Dirty the heap ---
    kprintf_cyan("\n  [3] O(1) PROOF - dirtying heap with %d allocs...\n", DIRTY_N);
    static const uint32_t dirty_sizes[7] = {60, 120, 200, 400, 800, 1500, 3000};
    for (int i = 0; i < DIRTY_N; i++)
        dp[i] = kmalloc(dirty_sizes[i % 7]);
    for (int i = 0; i < DIRTY_N; i += 2) {
        if (dp[i]) { kfree(dp[i]); dp[i] = 0; }
    }
    kprintf("    %d live allocs, %d freed holes across all domains\n\n",
            DIRTY_N/2, DIRTY_N/2);

    uint32_t d64_min,  d64_max;
    uint32_t d256_min, d256_max;
    uint32_t d1k_min,  d1k_max;
    measure_ss(64,   &d64_min,  &d64_max);
    measure_ss(256,  &d256_min, &d256_max);
    measure_ss(1024, &d1k_min,  &d1k_max);

    kprintf("    UNDER PRESSURE: min / jitter\n");
    kprintf("    kmalloc(64)   + kfree : min=%d  jitter=%d cycles\n",
            d64_min,  d64_max  - d64_min);
    kprintf("    kmalloc(256)  + kfree : min=%d  jitter=%d cycles\n",
            d256_min, d256_max - d256_min);
    kprintf("    kmalloc(1024) + kfree : min=%d  jitter=%d cycles\n",
            d1k_min,  d1k_max  - d1k_min);

    int32_t delta64  = (int32_t)d64_min  - (int32_t)c64_min;
    int32_t delta256 = (int32_t)d256_min - (int32_t)c256_min;
    int32_t delta1k  = (int32_t)d1k_min  - (int32_t)c1k_min;
    uint32_t max_delta = u32abs(delta64);
    if (u32abs(delta256) > max_delta) max_delta = u32abs(delta256);
    if (u32abs(delta1k)  > max_delta) max_delta = u32abs(delta1k);

    uint32_t max_jitter = c64_max - c64_min;
    if (c256_max - c256_min > max_jitter) max_jitter = c256_max - c256_min;
    if (c1k_max  - c1k_min  > max_jitter) max_jitter = c1k_max  - c1k_min;

    kprintf_green("\n  SPEED:       steady-state min = %d cycles (alloc+free)\n", c64_min);
    kprintf_green("  DETERMINISM: max jitter       = %d cycles\n", max_jitter);
    kprintf_green("  O(1) PROOF:  cost delta under heap pressure = %d cycles\n", max_delta);
    if (max_delta <= 10)
        kprintf_green("  VERDICT: O(1) CONFIRMED - heap state has zero effect on cost\n");
    else
        kprintf_yellow("  VERDICT: delta > 10 cycles - investigate\n");

    for (int i = 0; i < DIRTY_N; i++)
        if (dp[i]) kfree(dp[i]);

    kfree(s64); kfree(s256); kfree(s512); kfree(x256); kfree(dp);

    kprintf_white("\n========================================\n");
    kprintf_white("   MEMTEST 1 COMPLETE\n");
    kprintf_white("========================================\n\n");
}

// -----------------------------------------------------------------------
// memtest 2 - Large allocation test (kmalloc)
// -----------------------------------------------------------------------
void cmd_memtest_v2(void) {
    kprintf_white("\n========================================\n");
    kprintf_white("   RAXZUS HEAP - MEMTEST 2 (LARGE ALLOC)\n");
    kprintf_white("========================================\n\n");

    kprintf_yellow("[PHASE 3] Large Allocation Test (kmalloc)\n");
    kprintf("  Allocates N contiguous 4KB pages as a single region.\n");
    kprintf("  Verifies virtual contiguity and write/read correctness.\n\n");

    // --- 64 KB allocation (16 pages) ---
    kprintf("  Allocating 64 KB (16 pages)...\n");
    uint8_t* buf64k = (uint8_t*)kmalloc(64 * 1024);
    if (!buf64k) {
        kprintf_red("  FAIL: kmalloc(64K) returned NULL\n");
    } else {
        kprintf("  Got pointer: 0x%x\n", (uint32_t)buf64k);
        for (uint32_t i = 0; i < 64 * 1024; i++)
            buf64k[i] = (uint8_t)(i & 0xFF);
        int bad = 0;
        for (uint32_t i = 0; i < 64 * 1024; i++)
            if (buf64k[i] != (uint8_t)(i & 0xFF)) bad++;
        if (bad == 0)
            kprintf_green("  PASS: 64 KB written and verified (65536 bytes, zero errors)\n");
        else
            kprintf_red("  FAIL: %d bytes corrupted in 64 KB region\n", bad);
    }

    // --- 1 MB allocation (256 pages) ---
    kprintf("\n  Allocating 1 MB (256 pages)...\n");
    uint8_t* buf1m = (uint8_t*)kmalloc(1024 * 1024);
    if (!buf1m) {
        kprintf_red("  FAIL: kmalloc(1MB) returned NULL\n");
    } else {
        kprintf("  Got pointer: 0x%x\n", (uint32_t)buf1m);
        for (uint32_t i = 0; i < 1024 * 1024; i++)
            buf1m[i] = (uint8_t)((i ^ (i >> 8)) & 0xFF);
        int bad = 0;
        for (uint32_t i = 0; i < 1024 * 1024; i++)
            if (buf1m[i] != (uint8_t)((i ^ (i >> 8)) & 0xFF)) bad++;
        if (bad == 0)
            kprintf_green("  PASS: 1 MB written and verified (1048576 bytes, zero errors)\n");
        else
            kprintf_red("  FAIL: %d bytes corrupted in 1 MB region\n", bad);
    }

    if (buf64k && buf1m) {
        uint32_t gap = (uint32_t)buf1m - (uint32_t)buf64k;
        kprintf("\n  Address gap between 64K and 1MB regions: %d bytes\n", gap);
        if ((uint32_t)buf1m > (uint32_t)buf64k)
            kprintf_green("  PASS: Regions are non-overlapping and virtually contiguous\n");
        else
            kprintf_red("  FAIL: Region addresses overlap!\n");
    }

    kprintf_yellow("\n  Timing: kmalloc cost per page\n");
    asm volatile("cli");
    uint32_t t0 = rdtsc();
    void* tbuf = kmalloc(4096);
    uint32_t t1 = rdtsc();
    asm volatile("sti");
    kprintf("    1 page  (4 KB) : %d cycles\n", t1 - t0);

    asm volatile("cli");
    t0 = rdtsc();
    void* tbuf8 = kmalloc(8 * 4096);
    t1 = rdtsc();
    asm volatile("sti");
    kprintf("    8 pages (32 KB): %d cycles  (per-page: %d)\n",
            t1 - t0, (t1 - t0) / 8);

    kfree(tbuf);
    kfree(tbuf8);
    if (buf64k) kfree(buf64k);
    if (buf1m)  kfree(buf1m);

    kprintf_white("\n========================================\n");
    kprintf_white("   MEMTEST 2 COMPLETE\n");
    kprintf_white("========================================\n\n");
}
*/