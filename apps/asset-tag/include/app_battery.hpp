#ifndef APP_INCLUDE_APP_BATTERY_HPP
#define APP_INCLUDE_APP_BATTERY_HPP


/** A point in a battery discharge curve sequence.
 *
 * A discharge curve is defined as a sequence of these points, where
 * the first point has #lvl_pct set to 10000 and the last point has
 * #lvl_pct set to zero.  Both #lvl_pct and #lvl_mV should be
 * monotonic decreasing within the sequence.
 */
struct battery_level_point {
        /** Remaining life at #lvl_mV. */
        uint16_t lvl_pct;

        /** Battery voltage at #lvl_pct remaining life. */
        uint16_t lvl_mV;
};

/** A discharge curve specific to the power source. */
static constexpr battery_level_point levels[] = {
        /* Linear from maximum voltage to minimum voltage. */
        { 100, 3300 },
		{ 50, 3200},
        { 0, 2600 }, // Powered by mac gives 2.8V
};

/** Calculate the estimated battery level based on a measured voltage.
 *
 * @param batt_mV a measured battery voltage level.
 *
 * @param curve the discharge curve for the type of battery installed
 * on the system.
 *
 * @return the estimated remaining capacity in parts per ten
 * thousand.
 */
constexpr unsigned int battery_level_pct(unsigned int batt_mV) {
	const battery_level_point *pb = levels;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pct;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pct > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pct;
	}

	/* Linear interpolation between below and above points. */
	const battery_level_point *pa = pb - 1;

	return pb->lvl_pct + ((pa->lvl_pct - pb->lvl_pct) * (batt_mV - pb->lvl_mV) / (pa->lvl_mV - pb->lvl_mV));
}

#endif