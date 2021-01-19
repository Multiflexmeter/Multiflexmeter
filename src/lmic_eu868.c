#include "lmic/lmic_bandplan.h"
#include "rom_conf.h"
#include "config.h"

ostime_t LMICeulike_nextJoinState(uint8_t nDefaultChannels)
{
  ostime_t now = os_getTime();

  // Move txend to randomize synchronized concurrent joins.
  // Duty cycle is based on txend.
  ostime_t next_join_time = LMICbandplan_nextJoinTime(now);
  // Programmed allowed interval at this datarate
  // only applicable if MIN_LORA_DR is reached
  ostime_t measure_interval = sec2osticks(
      conf_getMeasurementInterval(LMIC.datarate));

  // Never fail a join, lower ever single try until the minimum
  // DR is reached, then stick at that DR using the preprogrammed
  // interval
  if (LMIC.datarate <= MIN_LORA_DR)
  {
    LMICcore_setDrJoin(DRCHG_NOJACC, MIN_LORA_DR);

    // If at the lowest DR and measure interval is later than
    // next TX opportunity. Then use measure_interval to delay
    // join attempt.
    if (next_join_time - now < measure_interval)
    {
      next_join_time = now + measure_interval;
    }
  }
  else
  {
    // As long as we arent at the lowest datarate, lower it and retry asap
    LMICcore_setDrJoin(DRCHG_NOJACC, decDR((dr_t)LMIC.datarate));
  }

  // Clear NEXTCHNL because join state engine controls channel hopping
  LMIC.opmode &= ~OP_NEXTCHNL;

  LMIC.txend = next_join_time +
               (isTESTMODE()
                    // Avoid collision with JOIN ACCEPT @ SF12 being sent by GW (but we missed it)
                    ? DNW2_SAFETY_ZONE
                    // Otherwise: randomize join (street lamp case):
                    // SF12:255, SF11:127, .., SF7:8secs
                    //
                    : DNW2_SAFETY_ZONE + LMICcore_rndDelay(255 >> LMIC.datarate));

  // Cannot fail in this implementation, keeps repeating at lowest DR forever
  // 1 - triggers EV_JOIN_FAILED event
  return 0;
}