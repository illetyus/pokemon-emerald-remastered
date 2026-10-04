#ifndef GUARD_CLOCK_H
#define GUARD_CLOCK_H

// TODO: time of day and seconds in a day defines

void DoTimeBasedEvents(void);
void RealignTimeBasedEventsAfterRtcCorrection(s32 days, s32 hours, s32 minutes);

#endif // GUARD_CLOCK_H
