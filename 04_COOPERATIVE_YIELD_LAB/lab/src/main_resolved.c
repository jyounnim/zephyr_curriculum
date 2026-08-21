// Source: 04_COOPERATIVE_YIELD_LAB.md

void coop_a_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopA: count=%ld\n", count);
            k_yield();   // voluntarily let CoopB (equal priority) have a turn
        }
    }
}

void coop_b_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopB: count=%ld\n", count);
            k_yield();
        }
    }
}
