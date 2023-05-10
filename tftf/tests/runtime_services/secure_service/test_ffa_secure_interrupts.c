/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <test_helpers.h>
#include <timer.h>

#define SENDER		HYP_ID
#define RECEIVER	SP_ID(1)
#define RECEIVER_2	SP_ID(2)
#define SP_SLEEP_TIME	1000U
#define NS_TIME_SLEEP	1500U
#define ECHO_VAL1	U(0xa0a0a0a0)

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}
	};

static event_t cpu_reached_end_of_test[PLATFORM_CORE_COUNT];

/*
 * @Test_Aim@ Test secure interrupt handling while first Secure Partition is
 * in RUNNING state. The interrupt is routed to the core defined by
 * PLAT_INTERRUPT_MPIDR and runs only on that core. If PLAT_INTERRUPT_MPIDR is
 * not defined, it defaults to current core.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Send a command to SP to first sleep( by executing a busy loop), then
 *    restart trusted watchdog timer and then sleep again.
 *
 * 3. While SP is running the first busy loop, Secure interrupt should trigger
 *    during this time.
 *
 * 4. The interrupt will be trapped to SPM as IRQ. SPM will inject the virtual
 *    IRQ to the first SP through vIRQ conduit and perform eret to resume
 *    execution in SP.
 *
 * 5. Execution traps to irq handler of Cactus SP. It will handle the secure
 *    interrupt triggered by the trusted watchdog timer.
 *
 * 6. Cactus SP will perform End-Of-Interrupt and resume execution in the busy
 *    loop.
 *
 * 7. Trusted watchdog timer will trigger once again followed by steps 4 to 6.
 *
 * 8. Cactus SP will send a direct response message with the elapsed time back
 *    to the normal world.
 *
 * 9. We make sure the time elapsed in the sleep routine by SP is not less than
 *    the requested value.
 *
 * 10. TFTF sends a direct request message to SP to query the ID of last serviced
 *     secure virtual interrupt.
 *
 * 11. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 12. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 *
 */
static test_result_t test_ffa_sec_interrupt_sp_running_handler(void)
{
	struct ffa_value ret_values;
	test_result_t test_ret = TEST_RESULT_FAIL;

#ifdef PLAT_INTERRUPT_MPIDR
	if (read_mpidr_el1() != PLAT_INTERRUPT_MPIDR) {
		test_ret = TEST_RESULT_SUCCESS;
		goto exit;
	}
#endif

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, 50);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		goto exit;
	}

	/* Send request to first Cactus SP to sleep */
	ret_values = cactus_sleep_trigger_wdog_cmd(SENDER, RECEIVER, SP_SLEEP_TIME, 50);

	/*
	 * Secure interrupt should trigger during this time, Cactus
	 * will handle the trusted watchdog timer.
	 */
	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for sleep command\n");
		goto exit;
	}

	VERBOSE("Secure interrupt has preempted execution: %u\n",
					cactus_get_response(ret_values));

	/* Make sure elapsed time not less than sleep time */
	if (cactus_get_response(ret_values) < SP_SLEEP_TIME) {
		ERROR("Lapsed time less than requested sleep time\n");
		goto exit;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		goto exit;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		goto exit;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	test_ret = TEST_RESULT_SUCCESS;
exit:
	if (test_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s - Test Failed - core - %d\n", __func__,
			get_current_core_id());
	}
	tftf_send_event(&cpu_reached_end_of_test[get_current_core_id()]);
	return test_ret;
}

test_result_t test_ffa_sec_interrupt_sp_running(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	memset(cpu_reached_end_of_test, 0, sizeof(cpu_reached_end_of_test));

#ifdef PLAT_INTERRUPT_MPIDR
	return spm_run_multi_core_test(
			(uintptr_t)test_ffa_sec_interrupt_sp_running_handler,
			cpu_reached_end_of_test);
#else
	return test_ffa_sec_interrupt_sp_running_handler();
#endif
}

/*
 * @Test_Aim@ Test secure interrupt handling while Secure Partition is waiting
 * for a message. The interrupt is routed to the core defined by
 * PLAT_INTERRUPT_MPIDR and runs only on that core. If PLAT_INTERRUPT_MPIDR is
 * not defined, it defaults to current core.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Once the SP returns with a direct response message, it moves to WAITING
      state.
 *
 * 3. Execute a busy loop to sleep for NS_TIME_SLEEP ms.
 *
 * 4. Trusted watchdog timer expires during this time which leads to secure
 *    interrupt being triggered while cpu is executing in normal world.
 *
 * 5. The interrupt is trapped to BL31/SPMD as FIQ and later synchronously
 *    delivered to SPM.
 *
 * 6. SPM injects a virtual IRQ to first Cactus Secure Partition.
 *
 * 7. Once the SP handles the interrupt, it returns execution back to normal
 *    world using FFA_MSG_WAIT call.
 *
 * 8. SPM, through the help of SPMD, resumes execution in normal world to
 *    continue the busy loop.
 *
 * 9. We make sure the time elapsed in the sleep routine is not less than
 *    the requested value.
 *
 * 10. TFTF sends a direct request message to SP to query the ID of last serviced
 *     secure virtual interrupt.
 *
 * 11. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 12. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 *
 */
static test_result_t test_ffa_sec_interrupt_sp_waiting_handler(void)
{
	uint64_t time1;
	volatile uint64_t time2, time_lapsed;
	uint64_t timer_freq = read_cntfrq_el0();
	struct ffa_value ret_values;
	test_result_t test_ret = TEST_RESULT_FAIL;

#ifdef PLAT_INTERRUPT_MPIDR
	if (read_mpidr_el1() != PLAT_INTERRUPT_MPIDR) {
		test_ret = TEST_RESULT_SUCCESS;
		goto exit;
	}
#endif

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	/*
	 * Send a message to SP1 through direct messaging.
	 */
	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, 100);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		goto exit;
	}

	time1 = syscounter_read();

	/*
	 * Sleep for NS_TIME_SLEEP ms. This ensures secure wdog timer triggers during this
	 * time. We explicitly do not use tftf_timer_sleep();
	 */
	waitms(NS_TIME_SLEEP);
	time2 = syscounter_read();

	/* Lapsed time should be at least equal to sleep time */
	time_lapsed = ((time2 - time1) * 1000) / timer_freq;

	if (time_lapsed < NS_TIME_SLEEP) {
		ERROR("Time elapsed less than expected value: %llu vs %u\n",
				time_lapsed, NS_TIME_SLEEP);
		goto exit;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		goto exit;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		goto exit;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	test_ret = TEST_RESULT_SUCCESS;
exit:
	if (test_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s - Test Failed - core - %d\n", __func__,
			get_current_core_id());
	}
	tftf_send_event(&cpu_reached_end_of_test[get_current_core_id()]);
	return test_ret;
}

test_result_t test_ffa_sec_interrupt_sp_waiting(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	memset(cpu_reached_end_of_test, 0, sizeof(cpu_reached_end_of_test));

#ifdef PLAT_INTERRUPT_MPIDR
	return spm_run_multi_core_test(
			(uintptr_t)test_ffa_sec_interrupt_sp_waiting_handler,
			cpu_reached_end_of_test);
#else
	return test_ffa_sec_interrupt_sp_waiting_handler();
#endif
}

/*
 * @Test_Aim@ Test secure interrupt handling while first Secure Partition is
 * in BLOCKED state. The interrupt is routed to the core defined by
 * PLAT_INTERRUPT_MPIDR and runs only on that core. If PLAT_INTERRUPT_MPIDR is
 * not defined, it defaults to current core.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Send a direct request to first SP to forward sleep command to second SP.
 *
 * 3. While second SP is running the busy loop, Secure interrupt should trigger
 *    during this time.
 *
 * 4. The interrupt will be trapped to SPM as IRQ. SPM will inject the virtual
 *    IRQ to the first SP through vIRQ conduit and perform eret to resume
 *    execution in first SP.
 *
 * 5. Execution traps to irq handler of Cactus SP. It will handle the secure
 *    interrupt triggered by the trusted watchdog timer.
 *
 * 6. First SP performs EOI by calling interrupt deactivate ABI and invokes
 *    FFA_RUN to resume second SP in the busy loop.
 *
 * 7. Second SP will complete the busy sleep loop and send a direct response
 *    message with the elapsed time back to the first SP.
 *
 * 8. First SP checks for the elapsed time and sends a direct response with
 *    a SUCCESS value back to tftf.
 *
 * 9. TFTF sends a direct request message to SP to query the ID of last serviced
 *    secure virtual interrupt.
 *
 * 10. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 11. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 */
static test_result_t test_ffa_sec_interrupt_sp_blocked_handler(void)
{
	struct ffa_value ret_values;
	test_result_t test_ret = TEST_RESULT_FAIL;

#ifdef PLAT_INTERRUPT_MPIDR
	if (read_mpidr_el1() != PLAT_INTERRUPT_MPIDR) {
		test_ret = TEST_RESULT_SUCCESS;
		goto exit;
	}
#endif

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, 100);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		goto exit;
	}

	/*
	 * Call FFA_RUN on the secondary vcpus to start it up. it is possible
	 * that another test has already started in, in which case we may get
	 * an error. We ignore the error and proceed, if the vcpu is not started
	 * the following direct requests would fail.
	 */
	ret_values = ffa_run(RECEIVER_2, get_current_core_id());
	if (ffa_func_id(ret_values) == FFA_ERROR) {
		WARN("Failed to start secondary vcpu of RECEIVER_2, "
			"may already be started by other tests. Ignoring"
			" error\n");
	}

	/*
	 * Send request to first Cactus SP to send request to Second Cactus
	 * SP to sleep
	 */
	ret_values = cactus_fwd_sleep_cmd(SENDER, RECEIVER, RECEIVER_2,
					 SP_SLEEP_TIME, false);

	/*
	 * Secure interrupt should trigger during this time, Cactus
	 * will handle the trusted watchdog timer.
	 */
	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response\n");
		goto exit;
	}

	if (cactus_get_response(ret_values) != CACTUS_SUCCESS) {
		ERROR("Expected CACTUS_SUCCESS %x\n", cactus_get_response(ret_values));
		goto exit;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		goto exit;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		goto exit;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	test_ret = TEST_RESULT_SUCCESS;
exit:
	if (test_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s - Test Failed - core - %d\n", __func__,
			get_current_core_id());
	}
	tftf_send_event(&cpu_reached_end_of_test[get_current_core_id()]);
	return test_ret;
}

test_result_t test_ffa_sec_interrupt_sp_blocked(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	memset(cpu_reached_end_of_test, 0, sizeof(cpu_reached_end_of_test));

#ifdef PLAT_INTERRUPT_MPIDR
	return spm_run_multi_core_test(
			(uintptr_t)test_ffa_sec_interrupt_sp_blocked_handler,
			cpu_reached_end_of_test);
#else
	return test_ffa_sec_interrupt_sp_blocked_handler();
#endif
}

/*
 * @Test_Aim@ Test secure interrupt handling while first Secure Partition is
 * in WAITING state while the second Secure Partition is running.
 * The interrupt is routed to the core defined by PLAT_INTERRUPT_MPIDR and
 * runs only on that core. If PLAT_INTERRUPT_MPIDR is not defined, it defaults
 * to current core.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Send a direct request to second SP to sleep by executing a busy loop.
 *
 * 3. While second SP is running the busy loop, Secure interrupt should trigger
 *    during this time.
 *
 * 4. The interrupt is trapped to the SPM as a physical IRQ. The SPM injects a
 *    virtual IRQ to the first SP and resumes it while it is in waiting state.
 *
 * 5. Execution traps to irq handler of the first Cactus SP. It will handle the
 *    secure interrupt triggered by the trusted watchdog timer.
 *
 * 6. Cactus SP will perform End-Of-Interrupt by calling the interrupt
 *    deactivate HVC and invoke FFA_MSG_WAIT ABI to perform interrupt signal
 *    completion.
 *
 * 7. SPM then resumes the second SP which was preempted by secure interrupt.
 *
 * 8. Second SP will complete the busy sleep loop and send a direct response
 *    message with the elapsed time back to the first SP.
 *
 * 9. We make sure the time elapsed in the sleep routine by SP is not less than
 *    the requested value.
 *
 * 10. TFTF sends a direct request message to SP to query the ID of last serviced
 *     secure virtual interrupt.
 *
 * 11. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 12. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 */
static test_result_t test_ffa_sec_interrupt_sp1_waiting_sp2_running_handler(void)
{
	struct ffa_value ret_values;
	test_result_t test_ret = TEST_RESULT_FAIL;

#ifdef PLAT_INTERRUPT_MPIDR
	if (read_mpidr_el1() != PLAT_INTERRUPT_MPIDR) {
		test_ret = TEST_RESULT_SUCCESS;
		goto exit;
	}
#endif

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, 100);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		goto exit;
	}

	/*
	 * Call FFA_RUN on the secondary vcpus to start it up. It is possible
	 * that another test has already started in, in which case we may get
	 * an error. We ignore the error and proceed, if the vcpu is not started
	 * the following direct requests would fail.
	 */
	ret_values = ffa_run(RECEIVER_2, get_current_core_id());
	if (ffa_func_id(ret_values) == FFA_ERROR) {
		WARN("Failed to start secondary vcpu of RECEIVER_2, "
			"may already be started by other tests. Ignoring"
			" error\n");
	}

	/* Send request to Second Cactus SP to sleep. */
	ret_values = cactus_sleep_cmd(SENDER, RECEIVER_2, SP_SLEEP_TIME);

	/*
	 * Secure interrupt should trigger during this time, Cactus
	 * will handle the trusted watchdog timer.
	 */
	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for sleep command\n");
		goto exit;
	}

	/* Make sure elapsed time not less than sleep time. */
	if (cactus_get_response(ret_values) < SP_SLEEP_TIME) {
		ERROR("Lapsed time less than requested sleep time\n");
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		goto exit;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		goto exit;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		goto exit;
	}

	test_ret = TEST_RESULT_SUCCESS;
exit:
	if (test_ret != TEST_RESULT_SUCCESS) {
		ERROR("%s - Test Failed - core - %d\n", __func__,
			get_current_core_id());
	}
	tftf_send_event(&cpu_reached_end_of_test[get_current_core_id()]);
	return test_ret;
}

test_result_t test_ffa_sec_interrupt_sp1_waiting_sp2_running(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	memset(cpu_reached_end_of_test, 0, sizeof(cpu_reached_end_of_test));

#ifdef PLAT_INTERRUPT_MPIDR
	return spm_run_multi_core_test(
			(uintptr_t)test_ffa_sec_interrupt_sp1_waiting_sp2_running_handler,
			cpu_reached_end_of_test);
#else
	return test_ffa_sec_interrupt_sp1_waiting_sp2_running_handler();
#endif
}
