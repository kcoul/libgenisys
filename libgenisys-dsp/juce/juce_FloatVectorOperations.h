/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#if USING_CHOWDHURY_EXTENSIONS
#include "xsimd/xsimd.hpp"
#endif

#include "juce_PlatformDefs.h"
#include "juce_Range.h"
#ifdef __ANDROID__
#define JUCE_USE_VDSP_FRAMEWORK 0
#elif __linux__
#define JUCE_USE_VDSP_FRAMEWORK 0
#include <cstdint>
#elif __APPLE__
#include <Accelerate/Accelerate.h>
#define JUCE_USE_VDSP_FRAMEWORK 1
#else
#endif

namespace juce
{

#ifndef JUCE_SNAP_TO_ZERO
#if JUCE_INTEL
#define JUCE_SNAP_TO_ZERO(n)            \
    if (!(n < -1.0e-8f || n > 1.0e-8f)) \
        n = 0;
#else
#define JUCE_SNAP_TO_ZERO(n) ignoreUnused(n)
#endif
#endif
class ScopedNoDenormals;

//==============================================================================
/**
    A collection of simple vector operations on arrays of floats, accelerated with
    SIMD instructions where possible.

    @tags{Audio}
*/
class FloatVectorOperations
{
public:
    //==============================================================================
    /** Clears a vector of floats. */
    static void clear(float* dest, int numValues) noexcept;

    /** Clears a vector of doubles. */
    static void clear(double* dest, int numValues) noexcept;

    /** Copies a repeated value into a vector of floats. */
    static void fill(float* dest, float valueToFill, int numValues) noexcept;

    /** Copies a repeated value into a vector of doubles. */
    static void fill(double* dest, double valueToFill, int numValues) noexcept;

    /** Copies a vector of floats. */
    static void copy(float* dest, const float* src, int numValues) noexcept;

    /** Copies a vector of doubles. */
    static void copy(double* dest, const double* src, int numValues) noexcept;

    /** Copies a vector of floats, multiplying each value by a given multiplier */
    static void copyWithMultiply(float* dest, const float* src, float multiplier, int numValues) noexcept;

    /** Copies a vector of doubles, multiplying each value by a given multiplier */
    static void copyWithMultiply(double* dest, const double* src, double multiplier, int numValues) noexcept;

    /** Adds a fixed value to the destination values. */
    static void add(float* dest, float amountToAdd, int numValues) noexcept;

    /** Adds a fixed value to the destination values. */
    static void add(double* dest, double amountToAdd, int numValues) noexcept;

    /** Adds a fixed value to each source value and stores it in the destination array. */
    static void add(float* dest, const float* src, float amount, int numValues) noexcept;

    /** Adds a fixed value to each source value and stores it in the destination array. */
    static void add(double* dest, const double* src, double amount, int numValues) noexcept;

    /** Adds the source values to the destination values. */
    static void add(float* dest, const float* src, int numValues) noexcept;

    /** Adds the source values to the destination values. */
    static void add(double* dest, const double* src, int numValues) noexcept;

    /** Adds each source1 value to the corresponding source2 value and stores the result in the destination array. */
    static void add(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Adds each source1 value to the corresponding source2 value and stores the result in the destination array. */
    static void add(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Subtracts the source values from the destination values. */
    static void subtract(float* dest, const float* src, int numValues) noexcept;

    /** Subtracts the source values from the destination values. */
    static void subtract(double* dest, const double* src, int numValues) noexcept;

    /** Subtracts each source2 value from the corresponding source1 value and stores the result in the destination array. */
    static void subtract(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Subtracts each source2 value from the corresponding source1 value and stores the result in the destination array. */
    static void subtract(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Multiplies each source value by the given multiplier, then adds it to the destination value. */
    static void addWithMultiply(float* dest, const float* src, float multiplier, int numValues) noexcept;

    /** Multiplies each source value by the given multiplier, then adds it to the destination value. */
    static void addWithMultiply(double* dest, const double* src, double multiplier, int numValues) noexcept;

    /** Multiplies each source1 value by the corresponding source2 value, then adds it to the destination value. */
    static void addWithMultiply(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Multiplies each source1 value by the corresponding source2 value, then adds it to the destination value. */
    static void addWithMultiply(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Multiplies each source value by the given multiplier, then subtracts it to the destination value. */
    static void subtractWithMultiply(float* dest, const float* src, float multiplier, int numValues) noexcept;

    /** Multiplies each source value by the given multiplier, then subtracts it to the destination value. */
    static void subtractWithMultiply(double* dest, const double* src, double multiplier, int numValues) noexcept;

    /** Multiplies each source1 value by the corresponding source2 value, then subtracts it to the destination value. */
    static void subtractWithMultiply(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Multiplies each source1 value by the corresponding source2 value, then subtracts it to the destination value. */
    static void subtractWithMultiply(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Multiplies the destination values by the source values. */
    static void multiply(float* dest, const float* src, int numValues) noexcept;

    /** Multiplies the destination values by the source values. */
    static void multiply(double* dest, const double* src, int numValues) noexcept;

    /** Multiplies each source1 value by the correspinding source2 value, then stores it in the destination array. */
    static void multiply(float* dest, const float* src1, const float* src2, int numValues) noexcept;

    /** Multiplies each source1 value by the correspinding source2 value, then stores it in the destination array. */
    static void multiply(double* dest, const double* src1, const double* src2, int numValues) noexcept;

    /** Multiplies each of the destination values by a fixed multiplier. */
    static void multiply(float* dest, float multiplier, int numValues) noexcept;

    /** Multiplies each of the destination values by a fixed multiplier. */
    static void multiply(double* dest, double multiplier, int numValues) noexcept;

    /** Multiplies each of the source values by a fixed multiplier and stores the result in the destination array. */
    static void multiply(float* dest, const float* src, float multiplier, int num) noexcept;

    /** Multiplies each of the source values by a fixed multiplier and stores the result in the destination array. */
    static void multiply(double* dest, const double* src, double multiplier, int num) noexcept;

    /** Copies a source vector to a destination, negating each value. */
    static void negate(float* dest, const float* src, int numValues) noexcept;

    /** Copies a source vector to a destination, negating each value. */
    static void negate(double* dest, const double* src, int numValues) noexcept;

    /** Copies a source vector to a destination, taking the absolute of each value. */
    static void abs(float* dest, const float* src, int numValues) noexcept;

    /** Copies a source vector to a destination, taking the absolute of each value. */
    static void abs(double* dest, const double* src, int numValues) noexcept;

    /** Converts a stream of integers to floats, multiplying each one by the given multiplier. */
    static void convertFixedToFloat(float* dest, const int* src, float multiplier, int numValues) noexcept;

    /** Each element of dest will be the minimum of the corresponding element of the source array and the given comp value. */
    static void min(float* dest, const float* src, float comp, int num) noexcept;

    /** Each element of dest will be the minimum of the corresponding element of the source array and the given comp value. */
    static void min(double* dest, const double* src, double comp, int num) noexcept;

    /** Each element of dest will be the minimum of the corresponding source1 and source2 values. */
    static void min(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Each element of dest will be the minimum of the corresponding source1 and source2 values. */
    static void min(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Each element of dest will be the maximum of the corresponding element of the source array and the given comp value. */
    static void max(float* dest, const float* src, float comp, int num) noexcept;

    /** Each element of dest will be the maximum of the corresponding element of the source array and the given comp value. */
    static void max(double* dest, const double* src, double comp, int num) noexcept;

    /** Each element of dest will be the maximum of the corresponding source1 and source2 values. */
    static void max(float* dest, const float* src1, const float* src2, int num) noexcept;

    /** Each element of dest will be the maximum of the corresponding source1 and source2 values. */
    static void max(double* dest, const double* src1, const double* src2, int num) noexcept;

    /** Each element of dest is calculated by hard clipping the corresponding src element so that it is in the range specified by the arguments low and high. */
    static void clip(float* dest, const float* src, float low, float high, int num) noexcept;

    /** Each element of dest is calculated by hard clipping the corresponding src element so that it is in the range specified by the arguments low and high. */
    static void clip(double* dest, const double* src, double low, double high, int num) noexcept;

    /** Finds the minimum and maximum values in the given array. */
    static Range<float> findMinAndMax(const float* src, int numValues) noexcept;

    /** Finds the minimum and maximum values in the given array. */
    static Range<double> findMinAndMax(const double* src, int numValues) noexcept;

    /** Finds the minimum value in the given array. */
    static float findMinimum(const float* src, int numValues) noexcept;

    /** Finds the minimum value in the given array. */
    static double findMinimum(const double* src, int numValues) noexcept;

    /** Finds the maximum value in the given array. */
    static float findMaximum(const float* src, int numValues) noexcept;

    /** Finds the maximum value in the given array. */
    static double findMaximum(const double* src, int numValues) noexcept;

    /** This method enables or disables the SSE/NEON flush-to-zero mode. */
    static void enableFlushToZeroMode(bool shouldEnable) noexcept;

    /** On Intel CPUs, this method enables the SSE flush-to-zero and denormalised-are-zero modes.
        This effectively sets the DAZ and FZ bits of the MXCSR register. On arm CPUs this will
        enable flush to zero mode.
        It's a convenient thing to call before audio processing code where you really want to
        avoid denormalisation performance hits.
    */
    static void disableDenormalisedNumberSupport(bool shouldDisable = true) noexcept;

    /** This method returns true if denormals are currently disabled. */
    static bool areDenormalsDisabled() noexcept;
#if USING_CHOWDHURY_EXTENSIONS
    /** Returns true if FloatVectorOperations will be performed using the Apple vDSP framework */
    static bool isUsingVDSP();

    /** Divides a scalar value by the src vector. */
    static void divide(float* dest, const float* dividend, const float* divisor, int numValues) noexcept;

    /** Divides a scalar value by the src vector. */
    static void divide(double* dest, const double* dividend, const double* divisor, int numValues) noexcept;

    /** Divides a scalar value by the src vector. */
    static void divide(float* dest, float dividend, const float* divisor, int numValues) noexcept;

    /** Divides a scalar value by the src vector. */
    static void divide(double* dest, double dividend, const double* divisor, int numValues) noexcept;

    /** Sums all the values in the given array. */
    static float accumulate(const float* src, int numValues) noexcept;

    /** Sums all the values in the given array. */
    static double accumulate(const double* src, int numValues) noexcept;

    /** Computes the inner product between the two arrays. */
    static float innerProduct(const float* src1, const float* src2, int numValues) noexcept;

    /** Computes the inner product between the two arrays. */
    static double innerProduct(const double* src1, const double* src2, int numValues) noexcept;

    /** Finds the absolute maximum value in the given array. */
    //static float findAbsoluteMaximum (const float* src, int numValues) noexcept;

    /** Finds the absolute maximum value in the given array. */
    //static double findAbsoluteMaximum (const double* src, int numValues) noexcept;

    /** Takes the exponent of each value to an integer power. */
    static void integerPower(float* dest, const float* src, int exponent, int numValues) noexcept;

    /** Takes the exponent of each value to an integer power. */
    static void integerPower(double* dest, const double* src, int exponent, int numValues) noexcept;

    /** Computes the Root-Mean-Square average of the input data. */
    static float computeRMS(const float* src, int numValues) noexcept;

    /** Computes the Root-Mean-Square average of the input data. */
    static double computeRMS(const double* src, int numValues) noexcept;
#endif
private:
    friend ScopedNoDenormals;

    static intptr_t getFpStatusRegister() noexcept;
    static void setFpStatusRegister(intptr_t) noexcept;
};

//==============================================================================
/**
     Helper class providing an RAII-based mechanism for temporarily disabling
     denormals on your CPU.

    @tags{Audio}
*/
class ScopedNoDenormals
{
public:
    ScopedNoDenormals() noexcept;
    ~ScopedNoDenormals() noexcept;

private:
#if JUCE_USE_SSE_INTRINSICS || (JUCE_USE_ARM_NEON || defined(__arm64__) || defined(__aarch64__))
    intptr_t fpsr;
#endif
};

} // namespace juce
