// Source: 17_CRITICAL_SECTION_LAB.md
// Section: 관찰 포인트

  struct k_spinlock lock;

  k_spinlock_key_t key = k_spin_lock(&lock);
  shared_counter++;
  k_spin_unlock(&lock, key);
  
