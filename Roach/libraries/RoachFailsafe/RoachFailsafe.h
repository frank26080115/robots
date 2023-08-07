#ifndef _ROACHFAILSAFE_H_
#define _ROACHFAILSAFE_H_

#include <stdint.h>
#include <stdbool.h>

void rfailsafe_init(uint32_t timeout);
void rfailsafe_task(uint32_t now);
void rfailsafe_feed(uint32_t now);
bool rfailsafe_isSafe(void);
void rfailsafe_attachOnFail(void(*cb)(void));
void rfailsafe_attachOnSafe(void(*cb)(void));

#endif
