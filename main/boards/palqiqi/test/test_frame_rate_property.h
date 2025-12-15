/**
 * @file test_frame_rate_property.h
 * @brief Header for frame rate property tests
 * 
 * **Feature: palqiqi-performance-optimization, Property 1: Frame Rate Maintenance During Speaking**
 * **Validates: Requirements 1.1**
 */

#pragma once

#ifdef CONFIG_PALQIQI_PERFORMANCE_TEST

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run all frame rate property tests
 * 
 * This function runs the property-based tests for frame rate maintenance
 * during speaking state. It uses the Unity test framework.
 */
void run_frame_rate_property_tests(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_PALQIQI_PERFORMANCE_TEST
