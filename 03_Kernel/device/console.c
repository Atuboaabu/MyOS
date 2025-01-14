#include "console.h"
#include "print.h"
#include "lock.h"

static struct lock console_lock;

void console_init() {
    lock_init(&console_lock);
}

void console_put_str(const uint8_t* str) {
    lock_acquire(&console_lock);
    put_str(str);
    lock_release(&console_lock);
}

void console_put_char(const uint8_t c) {
    lock_acquire(&console_lock);
    put_char(c);
    lock_release(&console_lock);
}

void console_put_int(const uint32_t num) {
    lock_acquire(&console_lock);
    put_int(num);
    lock_release(&console_lock);
}