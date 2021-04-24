#pragma once

void sleepq_init();
void sleepq_add(void* resource);
void sleepq_wake(void* resource);
void sleepq_wake_all(void* resource);