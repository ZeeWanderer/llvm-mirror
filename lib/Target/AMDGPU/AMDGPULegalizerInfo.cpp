//===- AMDGPULegalizerInfo.cpp -----------------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the Machinelegalizer class for
/// AMDGPU.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPULegalizerInfo.h"
#include "AMDGPUTargetMachine.h"
#include "SIMachineFunctionInfo.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace LegalizeActions;
using namespace LegalizeMutations;
using namespace LegalityPredicates;


static LegalityPredicate isMultiple32(unsigned TypeIdx,
                                      unsigned MaxSize = 512) {
  return [=](const LegalityQuery &Query) {
    const LLT Ty = Query.Types[TypeIdx];
    const LLT EltTy = Ty.getScalarType();
    return Ty.getSizeInBits() <= MaxSize && EltTy.getSizeInBits() % 32 == 0;
  };
}

static LegalityPredicate isSmallOddVector(unsigned TypeIdx) {
  return [=](const LegalityQuery &Query) {
    const LLT Ty = Query.Types[TypeIdx];
    return Ty.isVector() &&
           Ty.getNumElements() % 2 != 0 &&
           Ty.getElementType().getSizeInBits() < 32;
  };
}

static LegalizeMutation oneMoreElement(unsigned TypeIdx) {
  return [=](const LegalityQuery &Query) {
    const LLT Ty = Query.Types[TypeIdx];
    const LLT EltTy = Ty.getElementType();
    return std::make_pair(TypeIdx, LLT::vector(Ty.getNumElements() + 1, EltTy));
  };
}

static LegalizeMutation fewerEltsToSize64Vector(unsigned TypeIdx) {
  return [=](const LegalityQuery &Query) {
    const LLT Ty = Query.Types[TypeIdx];
    const LLT EltTy = Ty.getElementType();
    unsigned Size = Ty.getSizeInBits();
    unsigned Pieces = (Size + 63) / 64;
    unsigned NewNumElts = (Ty.getNumElements() + 1) / Pieces;
    return std::make_pair(TypeIdx, LLT::scalarOrVector(NewNumElts, EltTy));
  };
}

static LegalityPredicate vectorWiderThan(unsigned TypeIdx, unsigned Size) {
  return [=](const LegalityQuery &Query) {
    const LLT QueryTy = Query.Types[TypeIdx];
    return QueryTy.isVector() && QueryTy.getSizeInBits() > Size;
  };
}

static LegalityPredicate numElementsNotEven(unsigned TypeIdx) {
  return [=](const LegalityQuery &Query) {
    const LLT QueryTy = Query.Types[TypeIdx];
    return QueryTy.isVector() && QueryTy.getNumElements() % 2 != 0;
  };
}

AMDGPULegalizerInfo::AMDGPULegalizerInfo(const GCNSubtarget &ST,
                                         const GCNTargetMachine &TM) {
  using namespace TargetOpcode;

  auto GetAddrSpacePtr = [&TM](unsigned AS) {
    return LLT::pointer(AS, TM.getPointerSizeInBits(AS));
  };

  const LLT S1 = LLT::scalar(1);
  const LLT S8 = LLT::scalar(8);
  const LLT S16 = LLT::scalar(16);
  const LLT S32 = LLT::scalar(32);
  const LLT S64 = LLT::scalar(64);
  const LLT S128 = LLT::scalar(128);
  const LLT S256 = LLT::scalar(256);
  const LLT S512 = LLT::scalar(512);

  const LLT V2S16 = LLT::vector(2, 16);
  const LLT V4S16 = LLT::vector(4, 16);
  const LLT V8S16 = LLT::vector(8, 16);

  const LLT V2S32 = LLT::vector(2, 32);
  const LLT V3S32 = LLT::vector(3, 32);
  const LLT V4S32 = LLT::vector(4, 32);
  const LLT V5S32 = LLT::vector(5, 32);
  const LLT V6S32 = LLT::vector(6, 32);
  const LLT V7S32 = LLT::vector(7, 32);
  const LLT V8S32 = LLT::vector(8, 32);
  const LLT V9S32 = LLT::vector(9, 32);
  const LLT V10S32 = LLT::vector(10, 32);
  const LLT V11S32 = LLT::vector(11, 32);
  const LLT V12S32 = LLT::vector(12, 32);
  const LLT V13S32 = LLT::vector(13, 32);
  const LLT V14S32 = LLT::vector(14, 32);
  const LLT V15S32 = LLT::vector(15, 32);
  const LLT V16S32 = LLT::vector(16, 32);

  const LLT V2S64 = LLT::vector(2, 64);
  const LLT V3S64 = LLT::vector(3, 64);
  const LLT V4S64 = LLT::vector(4, 64);
  const LLT V5S64 = LLT::vector(5, 64);
  const LLT V6S64 = LLT::vector(6, 64);
  const LLT V7S64 = LLT::vector(7, 64);
  const LLT V8S64 = LLT::vector(8, 64);

  std::initializer_list<LLT> AllS32Vectors =
    {V2S32, V3S32, V4S32, V5S32, V6S32, V7S32, V8S32,
     V9S32, V10S32, V11S32, V12S32, V13S32, V14S32, V15S32, V16S32};
  std::initializer_list<LLT> AllS64Vectors =
    {V2S64, V3S64, V4S64, V5S64, V6S64, V7S64, V8S64};

  const LLT GlobalPtr = GetAddrSpacePtr(AMDGPUAS::GLOBAL_ADDRESS);
  const LLT ConstantPtr = GetAddrSpacePtr(AMDGPUAS::CONSTANT_ADDRESS);
  const LLT LocalPtr = GetAddrSpacePtr(AMDGPUAS::LOCAL_ADDRESS);
  const LLT FlatPtr = GetAddrSpacePtr(AMDGPUAS::FLAT_ADDRESS);
  const LLT PrivatePtr = GetAddrSpacePtr(AMDGPUAS::PRIVATE_ADDRESS);

  const LLT CodePtr = FlatPtr;

  const std::initializer_list<LLT> AddrSpaces64 = {
    GlobalPtr, ConstantPtr, FlatPtr
  };

  const std::initializer_list<LLT> AddrSpaces32 = {
    LocalPtr, PrivatePtr
  };

  const std::initializer_list<LLT> FPTypesBase = {
    S32, S64
  };

  const std::initializer_list<LLT> FPTypes16 = {
    S32, S64, S16
  };

  setAction({G_BRCOND, S1}, Legal);

  // TODO: All multiples of 32, vectors of pointers, all v2s16 pairs, more
  // elements for v3s16
  getActionDefinitionsBuilder(G_PHI)
    .legalFor({S32, S64, V2S16, V4S16, S1, S128, S256})
    .legalFor(AllS32Vectors)
    .legalFor(AllS64Vectors)
    .legalFor(AddrSpaces64)
    .legalFor(AddrSpaces32)
    .clampScalar(0, S32, S256)
    .widenScalarToNextPow2(0, 32)
    .clampMaxNumElements(0, S32, 16)
    .moreElementsIf(isSmallOddVector(0), oneMoreElement(0))
    .legalIf(isPointer(0));

  if (ST.has16BitInsts()) {
    getActionDefinitionsBuilder({G_ADD, G_SUB, G_MUL})
      .legalFor({S32, S16})
      .clampScalar(0, S16, S32)
      .scalarize(0);
  } else {
    getActionDefinitionsBuilder({G_ADD, G_SUB, G_MUL})
      .legalFor({S32})
      .clampScalar(0, S32, S32)
      .scalarize(0);
  }

  getActionDefinitionsBuilder({G_UMULH, G_SMULH})
    .legalFor({S32})
    .clampScalar(0, S32, S32)
    .scalarize(0);

  // Report legal for any types we can handle anywhere. For the cases only legal
  // on the SALU, RegBankSelect will be able to re-legalize.
  getActionDefinitionsBuilder({G_AND, G_OR, G_XOR})
    .legalFor({S32, S1, S64, V2S32, V2S16, V4S16})
    .clampScalar(0, S32, S64)
    .moreElementsIf(isSmallOddVector(0), oneMoreElement(0))
    .fewerElementsIf(vectorWiderThan(0, 32), fewerEltsToSize64Vector(0))
    .widenScalarToNextPow2(0)
    .scalarize(0);

  getActionDefinitionsBuilder({G_UADDO, G_SADDO, G_USUBO, G_SSUBO,
                               G_UADDE, G_SADDE, G_USUBE, G_SSUBE})
    .legalFor({{S32, S1}})
    .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(G_BITCAST)
    .legalForCartesianProduct({S32, V2S16})
    .legalForCartesianProduct({S64, V2S32, V4S16})
    .legalForCartesianProduct({V2S64, V4S32})
    // Don't worry about the size constraint.
    .legalIf(all(isPointer(0), isPointer(1)));

  if (ST.has16BitInsts()) {
    getActionDefinitionsBuilder(G_FCONSTANT)
      .legalFor({S32, S64, S16})
      .clampScalar(0, S16, S64);
  } else {
    getActionDefinitionsBuilder(G_FCONSTANT)
      .legalFor({S32, S64})
      .clampScalar(0, S32, S64);
  }

  getActionDefinitionsBuilder(G_IMPLICIT_DEF)
    .legalFor({S1, S32, S64, V2S32, V4S32, V2S16, V4S16, GlobalPtr,
               ConstantPtr, LocalPtr, FlatPtr, PrivatePtr})
    .moreElementsIf(isSmallOddVector(0), oneMoreElement(0))
    .clampScalarOrElt(0, S32, S512)
    .legalIf(isMultiple32(0))
    .widenScalarToNextPow2(0, 32)
    .clampMaxNumElements(0, S32, 16);


  // FIXME: i1 operands to intrinsics should always be legal, but other i1
  // values may not be legal.  We need to figure out how to distinguish
  // between these two scenarios.
  getActionDefinitionsBuilder(G_CONSTANT)
    .legalFor({S1, S32, S64, GlobalPtr,
               LocalPtr, ConstantPtr, PrivatePtr, FlatPtr })
    .clampScalar(0, S32, S64)
    .widenScalarToNextPow2(0)
    .legalIf(isPointer(0));

  setAction({G_FRAME_INDEX, PrivatePtr}, Legal);

  auto &FPOpActions = getActionDefinitionsBuilder(
    { G_FADD, G_FMUL, G_FNEG, G_FABS, G_FMA, G_FCANONICALIZE})
    .legalFor({S32, S64});

  if (ST.has16BitInsts()) {
    if (ST.hasVOP3PInsts())
      FPOpActions.legalFor({S16, V2S16});
    else
      FPOpActions.legalFor({S16});
  }

  if (ST.hasVOP3PInsts())
    FPOpActions.clampMaxNumElements(0, S16, 2);
  FPOpActions
    .scalarize(0)
    .clampScalar(0, ST.has16BitInsts() ? S16 : S32, S64);

  if (ST.has16BitInsts()) {
    getActionDefinitionsBuilder(G_FSQRT)
      .legalFor({S32, S64, S16})
      .scalarize(0)
      .clampScalar(0, S16, S64);
  } else {
    getActionDefinitionsBuilder(G_FSQRT)
      .legalFor({S32, S64})
      .scalarize(0)
      .clampScalar(0, S32, S64);
  }

  getActionDefinitionsBuilder(G_FPTRUNC)
    .legalFor({{S32, S64}, {S16, S32}})
    .scalarize(0);

  getActionDefinitionsBuilder(G_FPEXT)
    .legalFor({{S64, S32}, {S32, S16}})
    .lowerFor({{S64, S16}}) // FIXME: Implement
    .scalarize(0);

  getActionDefinitionsBuilder(G_FCOPYSIGN)
    .legalForCartesianProduct({S16, S32, S64}, {S16, S32, S64})
    .scalarize(0);

  getActionDefinitionsBuilder(G_FSUB)
      // Use actual fsub instruction
      .legalFor({S32})
      // Must use fadd + fneg
      .lowerFor({S64, S16, V2S16})
      .scalarize(0)
      .clampScalar(0, S32, S64);

  getActionDefinitionsBuilder({G_SEXT, G_ZEXT, G_ANYEXT})
    .legalFor({{S64, S32}, {S32, S16}, {S64, S16},
               {S32, S1}, {S64, S1}, {S16, S1},
               // FIXME: Hack
               {S64, LLT::scalar(33)},
               {S32, S8}, {S128, S32}, {S128, S64}, {S32, LLT::scalar(24)}})
    .scalarize(0);

  getActionDefinitionsBuilder({G_SITOFP, G_UITOFP})
    .legalFor({{S32, S32}, {S64, S32}})
    .lowerFor({{S32, S64}})
    .customFor({{S64, S64}})
    .scalarize(0);

  getActionDefinitionsBuilder({G_FPTOSI, G_FPTOUI})
    .legalFor({{S32, S32}, {S32, S64}})
    .scalarize(0);

  getActionDefinitionsBuilder(G_INTRINSIC_ROUND)
    .legalFor({S32, S64})
    .scalarize(0);

  if (ST.getGeneration() >= AMDGPUSubtarget::SEA_ISLANDS) {
    getActionDefinitionsBuilder({G_INTRINSIC_TRUNC, G_FCEIL, G_FRINT})
      .legalFor({S32, S64})
      .clampScalar(0, S32, S64)
      .scalarize(0);
  } else {
    getActionDefinitionsBuilder({G_INTRINSIC_TRUNC, G_FCEIL, G_FRINT})
      .legalFor({S32})
      .customFor({S64})
      .clampScalar(0, S32, S64)
      .scalarize(0);
  }

  getActionDefinitionsBuilder(G_GEP)
    .legalForCartesianProduct(AddrSpaces64, {S64})
    .legalForCartesianProduct(AddrSpaces32, {S32})
    .scalarize(0);

  setAction({G_BLOCK_ADDR, CodePtr}, Legal);

  getActionDefinitionsBuilder(G_ICMP)
    .legalForCartesianProduct(
      {S1}, {S32, S64, GlobalPtr, LocalPtr, ConstantPtr, PrivatePtr, FlatPtr})
    .legalFor({{S1, S32}, {S1, S64}})
    .widenScalarToNextPow2(1)
    .clampScalar(1, S32, S64)
    .scalarize(0)
    .legalIf(all(typeIs(0, S1), isPointer(1)));

  getActionDefinitionsBuilder(G_FCMP)
    .legalForCartesianProduct({S1}, ST.has16BitInsts() ? FPTypes16 : FPTypesBase)
    .widenScalarToNextPow2(1)
    .clampScalar(1, S32, S64)
    .scalarize(0);

  // FIXME: fexp, flog2, flog10 needs to be custom lowered.
  getActionDefinitionsBuilder({G_FPOW, G_FEXP, G_FEXP2,
                               G_FLOG, G_FLOG2, G_FLOG10})
    .legalFor({S32})
    .scalarize(0);

  // The 64-bit versions produce 32-bit results, but only on the SALU.
  getActionDefinitionsBuilder({G_CTLZ, G_CTLZ_ZERO_UNDEF,
                               G_CTTZ, G_CTTZ_ZERO_UNDEF,
                               G_CTPOP})
    .legalFor({{S32, S32}, {S32, S64}})
    .clampScalar(0, S32, S32)
    .clampScalar(1, S32, S64)
    .scalarize(0)
    .widenScalarToNextPow2(0, 32)
    .widenScalarToNextPow2(1, 32);

  // TODO: Expand for > s32
  getActionDefinitionsBuilder(G_BSWAP)
    .legalFor({S32})
    .clampScalar(0, S32, S32)
    .scalarize(0);

  if (ST.has16BitInsts()) {
    if (ST.hasVOP3PInsts()) {
      getActionDefinitionsBuilder({G_SMIN, G_SMAX, G_UMIN, G_UMAX})
        .legalFor({S32, S16, V2S16})
        .moreElementsIf(isSmallOddVector(0), oneMoreElement(0))
        .clampMaxNumElements(0, S16, 2)
        .clampScalar(0, S16, S32)
        .widenScalarToNextPow2(0)
        .scalarize(0);
    } else {
      getActionDefinitionsBuilder({G_SMIN, G_SMAX, G_UMIN, G_UMAX})
        .legalFor({S32, S16})
        .widenScalarToNextPow2(0)
        .clampScalar(0, S16, S32)
        .scalarize(0);
    }
  } else {
    getActionDefinitionsBuilder({G_SMIN, G_SMAX, G_UMIN, G_UMAX})
      .legalFor({S32})
      .clampScalar(0, S32, S32)
      .widenScalarToNextPow2(0)
      .scalarize(0);
  }

  auto smallerThan = [](unsigned TypeIdx0, unsigned TypeIdx1) {
    return [=](const LegalityQuery &Query) {
      return Query.Types[TypeIdx0].getSizeInBits() <
             Query.Types[TypeIdx1].getSizeInBits();
    };
  };

  auto greaterThan = [](unsigned TypeIdx0, unsigned TypeIdx1) {
    return [=](const LegalityQuery &Query) {
      return Query.Types[TypeIdx0].getSizeInBits() >
             Query.Types[TypeIdx1].getSizeInBits();
    };
  };

  getActionDefinitionsBuilder(G_INTTOPTR)
    // List the common cases
    .legalForCartesianProduct(AddrSpaces64, {S64})
    .legalForCartesianProduct(AddrSpaces32, {S32})
    .scalarize(0)
    // Accept any address space as long as the size matches
    .legalIf(sameSize(0, 1))
    .widenScalarIf(smallerThan(1, 0),
      [](const LegalityQuery &Query) {
        return std::make_pair(1, LLT::scalar(Query.Types[0].getSizeInBits()));
      })
    .narrowScalarIf(greaterThan(1, 0),
      [](const LegalityQuery &Query) {
        return std::make_pair(1, LLT::scalar(Query.Types[0].getSizeInBits()));
      });

  getActionDefinitionsBuilder(G_PTRTOINT)
    // List the common cases
    .legalForCartesianProduct(AddrSpaces64, {S64})
    .legalForCartesianProduct(AddrSpaces32, {S32})
    .scalarize(0)
    // Accept any address space as long as the size matches
    .legalIf(sameSize(0, 1))
    .widenScalarIf(smallerThan(0, 1),
      [](const LegalityQuery &Query) {
        return std::make_pair(0, LLT::scalar(Query.Types[1].getSizeInBits()));
      })
    .narrowScalarIf(
      greaterThan(0, 1),
      [](const LegalityQuery &Query) {
        return std::make_pair(0, LLT::scalar(Query.Types[1].getSizeInBits()));
      });

  if (ST.hasFlatAddressSpace()) {
    getActionDefinitionsBuilder(G_ADDRSPACE_CAST)
      .scalarize(0)
      .custom();
  }

  getActionDefinitionsBuilder({G_LOAD, G_STORE})
    .narrowScalarIf([](const LegalityQuery &Query) {
        unsigned Size = Query.Types[0].getSizeInBits();
        unsigned MemSize = Query.MMODescrs[0].SizeInBits;
        return (Size > 32 && MemSize < Size);
      },
      [](const LegalityQuery &Query) {
        return std::make_pair(0, LLT::scalar(32));
      })
    .fewerElementsIf([=, &ST](const LegalityQuery &Query) {
        unsigned MemSize = Query.MMODescrs[0].SizeInBits;
        return (MemSize == 96) &&
               Query.Types[0].isVector() &&
               !ST.hasDwordx3LoadStores();
      },
      [=](const LegalityQuery &Query) {
        return std::make_pair(0, V2S32);
      })
    .legalIf([=, &ST](const LegalityQuery &Query) {
        const LLT &Ty0 = Query.Types[0];

        unsigned Size = Ty0.getSizeInBits();
        unsigned MemSize = Query.MMODescrs[0].SizeInBits;
        if (Size < 32 || (Size > 32 && MemSize < Size))
          return false;

        if (Ty0.isVector() && Size != MemSize)
          return false;

        // TODO: Decompose private loads into 4-byte components.
        // TODO: Illegal flat loads on SI
        switch (MemSize) {
        case 8:
        case 16:
          return Size == 32;
        case 32:
        case 64:
        case 128:
          return true;

        case 96:
          return ST.hasDwordx3LoadStores();

        case 256:
        case 512:
          // TODO: constant loads
        default:
          return false;
        }
      })
    .clampScalar(0, S32, S64);


  // FIXME: Handle alignment requirements.
  auto &ExtLoads = getActionDefinitionsBuilder({G_SEXTLOAD, G_ZEXTLOAD})
    .legalForTypesWithMemDesc({
        {S32, GlobalPtr, 8, 8},
        {S32, GlobalPtr, 16, 8},
        {S32, LocalPtr, 8, 8},
        {S32, LocalPtr, 16, 8},
        {S32, PrivatePtr, 8, 8},
        {S32, PrivatePtr, 16, 8}});
  if (ST.hasFlatAddressSpace()) {
    ExtLoads.legalForTypesWithMemDesc({{S32, FlatPtr, 8, 8},
                                       {S32, FlatPtr, 16, 8}});
  }

  ExtLoads.clampScalar(0, S32, S32)
          .widenScalarToNextPow2(0)
          .unsupportedIfMemSizeNotPow2()
          .lower();

  auto &Atomics = getActionDefinitionsBuilder(
    {G_ATOMICRMW_XCHG, G_ATOMICRMW_ADD, G_ATOMICRMW_SUB,
     G_ATOMICRMW_AND, G_ATOMICRMW_OR, G_ATOMICRMW_XOR,
     G_ATOMICRMW_MAX, G_ATOMICRMW_MIN, G_ATOMICRMW_UMAX,
     G_ATOMICRMW_UMIN, G_ATOMIC_CMPXCHG})
    .legalFor({{S32, GlobalPtr}, {S32, LocalPtr},
               {S64, GlobalPtr}, {S64, LocalPtr}});
  if (ST.hasFlatAddressSpace()) {
    Atomics.legalFor({{S32, FlatPtr}, {S64, FlatPtr}});
  }

  // TODO: Pointer types, any 32-bit or 64-bit vector
  getActionDefinitionsBuilder(G_SELECT)
    .legalForCartesianProduct({S32, S64, S16, V2S32, V2S16, V4S16,
          GlobalPtr, LocalPtr, FlatPtr, PrivatePtr,
          LLT::vector(2, LocalPtr), LLT::vector(2, PrivatePtr)}, {S1})
    .clampScalar(0, S16, S64)
    .moreElementsIf(isSmallOddVector(0), oneMoreElement(0))
    .fewerElementsIf(numElementsNotEven(0), scalarize(0))
    .scalarize(1)
    .clampMaxNumElements(0, S32, 2)
    .clampMaxNumElements(0, LocalPtr, 2)
    .clampMaxNumElements(0, PrivatePtr, 2)
    .scalarize(0)
    .widenScalarToNextPow2(0)
    .legalIf(all(isPointer(0), typeIs(1, S1)));

  // TODO: Only the low 4/5/6 bits of the shift amount are observed, so we can
  // be more flexible with the shift amount type.
  auto &Shifts = getActionDefinitionsBuilder({G_SHL, G_LSHR, G_ASHR})
    .legalFor({{S32, S32}, {S64, S32}});
  if (ST.has16BitInsts()) {
    if (ST.hasVOP3PInsts()) {
      Shifts.legalFor({{S16, S32}, {S16, S16}, {V2S16, V2S16}})
            .clampMaxNumElements(0, S16, 2);
    } else
      Shifts.legalFor({{S16, S32}, {S16, S16}});

    Shifts.clampScalar(1, S16, S32);
    Shifts.clampScalar(0, S16, S64);
    Shifts.widenScalarToNextPow2(0, 16);
  } else {
    // Make sure we legalize the shift amount type first, as the general
    // expansion for the shifted type will produce much worse code if it hasn't
    // been truncated already.
    Shifts.clampScalar(1, S32, S32);
    Shifts.clampScalar(0, S32, S64);
    Shifts.widenScalarToNextPow2(0, 32);
  }
  Shifts.scalarize(0);

  for (unsigned Op : {G_EXTRACT_VECTOR_ELT, G_INSERT_VECTOR_ELT}) {
    unsigned VecTypeIdx = Op == G_EXTRACT_VECTOR_ELT ? 1 : 0;
    unsigned EltTypeIdx = Op == G_EXTRACT_VECTOR_ELT ? 0 : 1;
    unsigned IdxTypeIdx = 2;

    getActionDefinitionsBuilder(Op)
      .legalIf([=](const LegalityQuery &Query) {
          const LLT &VecTy = Query.Types[VecTypeIdx];
          const LLT &IdxTy = Query.Types[IdxTypeIdx];
          return VecTy.getSizeInBits() % 32 == 0 &&
            VecTy.getSizeInBits() <= 512 &&
            IdxTy.getSizeInBits() == 32;
        })
      .clampScalar(EltTypeIdx, S32, S64)
      .clampScalar(VecTypeIdx, S32, S64)
      .clampScalar(IdxTypeIdx, S32, S32);
  }

  getActionDefinitionsBuilder(G_EXTRACT_VECTOR_ELT)
    .unsupportedIf([=](const LegalityQuery &Query) {
        const LLT &EltTy = Query.Types[1].getElementType();
        return Query.Types[0] != EltTy;
      });

  for (unsigned Op : {G_EXTRACT, G_INSERT}) {
    unsigned BigTyIdx = Op == G_EXTRACT ? 1 : 0;
    unsigned LitTyIdx = Op == G_EXTRACT ? 0 : 1;

    // FIXME: Doesn't handle extract of illegal sizes.
    getActionDefinitionsBuilder(Op)
      .legalIf([=](const LegalityQuery &Query) {
          const LLT BigTy = Query.Types[BigTyIdx];
          const LLT LitTy = Query.Types[LitTyIdx];
          return (BigTy.getSizeInBits() % 32 == 0) &&
                 (LitTy.getSizeInBits() % 16 == 0);
        })
      .widenScalarIf(
        [=](const LegalityQuery &Query) {
          const LLT BigTy = Query.Types[BigTyIdx];
          return (BigTy.getScalarSizeInBits() < 16);
        },
        LegalizeMutations::widenScalarOrEltToNextPow2(BigTyIdx, 16))
      .widenScalarIf(
        [=](const LegalityQuery &Query) {
          const LLT LitTy = Query.Types[LitTyIdx];
          return (LitTy.getScalarSizeInBits() < 16);
        },
        LegalizeMutations::widenScalarOrEltToNextPow2(LitTyIdx, 16))
      .moreElementsIf(isSmallOddVector(BigTyIdx), oneMoreElement(BigTyIdx))
      .widenScalarToNextPow2(BigTyIdx, 32);

  }

  // TODO: vectors of pointers
  getActionDefinitionsBuilder(G_BUILD_VECTOR)
      .legalForCartesianProduct(AllS32Vectors, {S32})
      .legalForCartesianProduct(AllS64Vectors, {S64})
      .clampNumElements(0, V16S32, V16S32)
      .clampNumElements(0, V2S64, V8S64)
      .minScalarSameAs(1, 0)
      // FIXME: Sort of a hack to make progress on other legalizations.
      .legalIf([=](const LegalityQuery &Query) {
        return Query.Types[0].getScalarSizeInBits() <= 32 ||
               Query.Types[0].getScalarSizeInBits() == 64;
      });

  // TODO: Support any combination of v2s32
  getActionDefinitionsBuilder(G_CONCAT_VECTORS)
    .legalFor({{V4S32, V2S32},
               {V8S32, V2S32},
               {V8S32, V4S32},
               {V4S64, V2S64},
               {V4S16, V2S16},
               {V8S16, V2S16},
               {V8S16, V4S16},
               {LLT::vector(4, LocalPtr), LLT::vector(2, LocalPtr)},
               {LLT::vector(4, PrivatePtr), LLT::vector(2, PrivatePtr)}});

  // Merge/Unmerge
  for (unsigned Op : {G_MERGE_VALUES, G_UNMERGE_VALUES}) {
    unsigned BigTyIdx = Op == G_MERGE_VALUES ? 0 : 1;
    unsigned LitTyIdx = Op == G_MERGE_VALUES ? 1 : 0;

    auto notValidElt = [=](const LegalityQuery &Query, unsigned TypeIdx) {
      const LLT &Ty = Query.Types[TypeIdx];
      if (Ty.isVector()) {
        const LLT &EltTy = Ty.getElementType();
        if (EltTy.getSizeInBits() < 8 || EltTy.getSizeInBits() > 64)
          return true;
        if (!isPowerOf2_32(EltTy.getSizeInBits()))
          return true;
      }
      return false;
    };

    getActionDefinitionsBuilder(Op)
      .widenScalarToNextPow2(LitTyIdx, /*Min*/ 16)
      // Clamp the little scalar to s8-s256 and make it a power of 2. It's not
      // worth considering the multiples of 64 since 2*192 and 2*384 are not
      // valid.
      .clampScalar(LitTyIdx, S16, S256)
      .widenScalarToNextPow2(LitTyIdx, /*Min*/ 32)

      // Break up vectors with weird elements into scalars
      .fewerElementsIf(
        [=](const LegalityQuery &Query) { return notValidElt(Query, 0); },
        scalarize(0))
      .fewerElementsIf(
        [=](const LegalityQuery &Query) { return notValidElt(Query, 1); },
        scalarize(1))
      .clampScalar(BigTyIdx, S32, S512)
      .widenScalarIf(
        [=](const LegalityQuery &Query) {
          const LLT &Ty = Query.Types[BigTyIdx];
          return !isPowerOf2_32(Ty.getSizeInBits()) &&
                 Ty.getSizeInBits() % 16 != 0;
        },
        [=](const LegalityQuery &Query) {
          // Pick the next power of 2, or a multiple of 64 over 128.
          // Whichever is smaller.
          const LLT &Ty = Query.Types[BigTyIdx];
          unsigned NewSizeInBits = 1 << Log2_32_Ceil(Ty.getSizeInBits() + 1);
          if (NewSizeInBits >= 256) {
            unsigned RoundedTo = alignTo<64>(Ty.getSizeInBits() + 1);
            if (RoundedTo < NewSizeInBits)
              NewSizeInBits = RoundedTo;
          }
          return std::make_pair(BigTyIdx, LLT::scalar(NewSizeInBits));
        })
      .legalIf([=](const LegalityQuery &Query) {
          const LLT &BigTy = Query.Types[BigTyIdx];
          const LLT &LitTy = Query.Types[LitTyIdx];

          if (BigTy.isVector() && BigTy.getSizeInBits() < 32)
            return false;
          if (LitTy.isVector() && LitTy.getSizeInBits() < 32)
            return false;

          return BigTy.getSizeInBits() % 16 == 0 &&
                 LitTy.getSizeInBits() % 16 == 0 &&
                 BigTy.getSizeInBits() <= 512;
        })
      // Any vectors left are the wrong size. Scalarize them.
      .scalarize(0)
      .scalarize(1);
  }

  computeTables();
  verify(*ST.getInstrInfo());
}

bool AMDGPULegalizerInfo::legalizeCustom(MachineInstr &MI,
                                         MachineRegisterInfo &MRI,
                                         MachineIRBuilder &MIRBuilder,
                                         GISelChangeObserver &Observer) const {
  switch (MI.getOpcode()) {
  case TargetOpcode::G_ADDRSPACE_CAST:
    return legalizeAddrSpaceCast(MI, MRI, MIRBuilder);
  case TargetOpcode::G_FRINT:
    return legalizeFrint(MI, MRI, MIRBuilder);
  case TargetOpcode::G_FCEIL:
    return legalizeFceil(MI, MRI, MIRBuilder);
  case TargetOpcode::G_INTRINSIC_TRUNC:
    return legalizeIntrinsicTrunc(MI, MRI, MIRBuilder);
  case TargetOpcode::G_SITOFP:
    return legalizeITOFP(MI, MRI, MIRBuilder, true);
  case TargetOpcode::G_UITOFP:
    return legalizeITOFP(MI, MRI, MIRBuilder, false);
  default:
    return false;
  }

  llvm_unreachable("expected switch to return");
}

Register AMDGPULegalizerInfo::getSegmentAperture(
  unsigned AS,
  MachineRegisterInfo &MRI,
  MachineIRBuilder &MIRBuilder) const {
  MachineFunction &MF = MIRBuilder.getMF();
  const GCNSubtarget &ST = MF.getSubtarget<GCNSubtarget>();
  const LLT S32 = LLT::scalar(32);

  if (ST.hasApertureRegs()) {
    // FIXME: Use inline constants (src_{shared, private}_base) instead of
    // getreg.
    unsigned Offset = AS == AMDGPUAS::LOCAL_ADDRESS ?
        AMDGPU::Hwreg::OFFSET_SRC_SHARED_BASE :
        AMDGPU::Hwreg::OFFSET_SRC_PRIVATE_BASE;
    unsigned WidthM1 = AS == AMDGPUAS::LOCAL_ADDRESS ?
        AMDGPU::Hwreg::WIDTH_M1_SRC_SHARED_BASE :
        AMDGPU::Hwreg::WIDTH_M1_SRC_PRIVATE_BASE;
    unsigned Encoding =
        AMDGPU::Hwreg::ID_MEM_BASES << AMDGPU::Hwreg::ID_SHIFT_ |
        Offset << AMDGPU::Hwreg::OFFSET_SHIFT_ |
        WidthM1 << AMDGPU::Hwreg::WIDTH_M1_SHIFT_;

    Register ApertureReg = MRI.createGenericVirtualRegister(S32);
    Register GetReg = MRI.createVirtualRegister(&AMDGPU::SReg_32RegClass);

    MIRBuilder.buildInstr(AMDGPU::S_GETREG_B32)
      .addDef(GetReg)
      .addImm(Encoding);
    MRI.setType(GetReg, S32);

    auto ShiftAmt = MIRBuilder.buildConstant(S32, WidthM1 + 1);
    MIRBuilder.buildInstr(TargetOpcode::G_SHL)
      .addDef(ApertureReg)
      .addUse(GetReg)
      .addUse(ShiftAmt.getReg(0));

    return ApertureReg;
  }

  Register QueuePtr = MRI.createGenericVirtualRegister(
    LLT::pointer(AMDGPUAS::CONSTANT_ADDRESS, 64));

  // FIXME: Placeholder until we can track the input registers.
  MIRBuilder.buildConstant(QueuePtr, 0xdeadbeef);

  // Offset into amd_queue_t for group_segment_aperture_base_hi /
  // private_segment_aperture_base_hi.
  uint32_t StructOffset = (AS == AMDGPUAS::LOCAL_ADDRESS) ? 0x40 : 0x44;

  // FIXME: Don't use undef
  Value *V = UndefValue::get(PointerType::get(
                               Type::getInt8Ty(MF.getFunction().getContext()),
                               AMDGPUAS::CONSTANT_ADDRESS));

  MachinePointerInfo PtrInfo(V, StructOffset);
  MachineMemOperand *MMO = MF.getMachineMemOperand(
    PtrInfo,
    MachineMemOperand::MOLoad |
    MachineMemOperand::MODereferenceable |
    MachineMemOperand::MOInvariant,
    4,
    MinAlign(64, StructOffset));

  Register LoadResult = MRI.createGenericVirtualRegister(S32);
  Register LoadAddr;

  MIRBuilder.materializeGEP(LoadAddr, QueuePtr, LLT::scalar(64), StructOffset);
  MIRBuilder.buildLoad(LoadResult, LoadAddr, *MMO);
  return LoadResult;
}

bool AMDGPULegalizerInfo::legalizeAddrSpaceCast(
  MachineInstr &MI, MachineRegisterInfo &MRI,
  MachineIRBuilder &MIRBuilder) const {
  MachineFunction &MF = MIRBuilder.getMF();

  MIRBuilder.setInstr(MI);

  Register Dst = MI.getOperand(0).getReg();
  Register Src = MI.getOperand(1).getReg();

  LLT DstTy = MRI.getType(Dst);
  LLT SrcTy = MRI.getType(Src);
  unsigned DestAS = DstTy.getAddressSpace();
  unsigned SrcAS = SrcTy.getAddressSpace();

  // TODO: Avoid reloading from the queue ptr for each cast, or at least each
  // vector element.
  assert(!DstTy.isVector());

  const AMDGPUTargetMachine &TM
    = static_cast<const AMDGPUTargetMachine &>(MF.getTarget());

  const GCNSubtarget &ST = MF.getSubtarget<GCNSubtarget>();
  if (ST.getTargetLowering()->isNoopAddrSpaceCast(SrcAS, DestAS)) {
    MI.setDesc(MIRBuilder.getTII().get(TargetOpcode::G_BITCAST));
    return true;
  }

  if (SrcAS == AMDGPUAS::FLAT_ADDRESS) {
    assert(DestAS == AMDGPUAS::LOCAL_ADDRESS ||
           DestAS == AMDGPUAS::PRIVATE_ADDRESS);
    unsigned NullVal = TM.getNullPointerValue(DestAS);

    auto SegmentNull = MIRBuilder.buildConstant(DstTy, NullVal);
    auto FlatNull = MIRBuilder.buildConstant(SrcTy, 0);

    Register PtrLo32 = MRI.createGenericVirtualRegister(DstTy);

    // Extract low 32-bits of the pointer.
    MIRBuilder.buildExtract(PtrLo32, Src, 0);

    Register CmpRes = MRI.createGenericVirtualRegister(LLT::scalar(1));
    MIRBuilder.buildICmp(CmpInst::ICMP_NE, CmpRes, Src, FlatNull.getReg(0));
    MIRBuilder.buildSelect(Dst, CmpRes, PtrLo32, SegmentNull.getReg(0));

    MI.eraseFromParent();
    return true;
  }

  assert(SrcAS == AMDGPUAS::LOCAL_ADDRESS ||
         SrcAS == AMDGPUAS::PRIVATE_ADDRESS);

  auto SegmentNull =
      MIRBuilder.buildConstant(SrcTy, TM.getNullPointerValue(SrcAS));
  auto FlatNull =
      MIRBuilder.buildConstant(DstTy, TM.getNullPointerValue(DestAS));

  Register ApertureReg = getSegmentAperture(DestAS, MRI, MIRBuilder);

  Register CmpRes = MRI.createGenericVirtualRegister(LLT::scalar(1));
  MIRBuilder.buildICmp(CmpInst::ICMP_NE, CmpRes, Src, SegmentNull.getReg(0));

  Register BuildPtr = MRI.createGenericVirtualRegister(DstTy);

  // Coerce the type of the low half of the result so we can use merge_values.
  Register SrcAsInt = MRI.createGenericVirtualRegister(LLT::scalar(32));
  MIRBuilder.buildInstr(TargetOpcode::G_PTRTOINT)
    .addDef(SrcAsInt)
    .addUse(Src);

  // TODO: Should we allow mismatched types but matching sizes in merges to
  // avoid the ptrtoint?
  MIRBuilder.buildMerge(BuildPtr, {SrcAsInt, ApertureReg});
  MIRBuilder.buildSelect(Dst, CmpRes, BuildPtr, FlatNull.getReg(0));

  MI.eraseFromParent();
  return true;
}

bool AMDGPULegalizerInfo::legalizeFrint(
  MachineInstr &MI, MachineRegisterInfo &MRI,
  MachineIRBuilder &MIRBuilder) const {
  MIRBuilder.setInstr(MI);

  Register Src = MI.getOperand(1).getReg();
  LLT Ty = MRI.getType(Src);
  assert(Ty.isScalar() && Ty.getSizeInBits() == 64);

  APFloat C1Val(APFloat::IEEEdouble(), "0x1.0p+52");
  APFloat C2Val(APFloat::IEEEdouble(), "0x1.fffffffffffffp+51");

  auto C1 = MIRBuilder.buildFConstant(Ty, C1Val);
  auto CopySign = MIRBuilder.buildFCopysign(Ty, C1, Src);

  // TODO: Should this propagate fast-math-flags?
  auto Tmp1 = MIRBuilder.buildFAdd(Ty, Src, CopySign);
  auto Tmp2 = MIRBuilder.buildFSub(Ty, Tmp1, CopySign);

  auto C2 = MIRBuilder.buildFConstant(Ty, C2Val);
  auto Fabs = MIRBuilder.buildFAbs(Ty, Src);

  auto Cond = MIRBuilder.buildFCmp(CmpInst::FCMP_OGT, LLT::scalar(1), Fabs, C2);
  MIRBuilder.buildSelect(MI.getOperand(0).getReg(), Cond, Src, Tmp2);
  return true;
}

bool AMDGPULegalizerInfo::legalizeFceil(
  MachineInstr &MI, MachineRegisterInfo &MRI,
  MachineIRBuilder &B) const {
  B.setInstr(MI);

  const LLT S1 = LLT::scalar(1);
  const LLT S64 = LLT::scalar(64);

  Register Src = MI.getOperand(1).getReg();
  assert(MRI.getType(Src) == S64);

  // result = trunc(src)
  // if (src > 0.0 && src != result)
  //   result += 1.0

  auto Trunc = B.buildInstr(TargetOpcode::G_INTRINSIC_TRUNC, {S64}, {Src});

  const auto Zero = B.buildFConstant(S64, 0.0);
  const auto One = B.buildFConstant(S64, 1.0);
  auto Lt0 = B.buildFCmp(CmpInst::FCMP_OGT, S1, Src, Zero);
  auto NeTrunc = B.buildFCmp(CmpInst::FCMP_ONE, S1, Src, Trunc);
  auto And = B.buildAnd(S1, Lt0, NeTrunc);
  auto Add = B.buildSelect(S64, And, One, Zero);

  // TODO: Should this propagate fast-math-flags?
  B.buildFAdd(MI.getOperand(0).getReg(), Trunc, Add);
  return true;
}

static MachineInstrBuilder extractF64Exponent(unsigned Hi,
                                              MachineIRBuilder &B) {
  const unsigned FractBits = 52;
  const unsigned ExpBits = 11;
  LLT S32 = LLT::scalar(32);

  auto Const0 = B.buildConstant(S32, FractBits - 32);
  auto Const1 = B.buildConstant(S32, ExpBits);

  auto ExpPart = B.buildIntrinsic(Intrinsic::amdgcn_ubfe, {S32}, false)
    .addUse(Const0.getReg(0))
    .addUse(Const1.getReg(0));

  return B.buildSub(S32, ExpPart, B.buildConstant(S32, 1023));
}

bool AMDGPULegalizerInfo::legalizeIntrinsicTrunc(
  MachineInstr &MI, MachineRegisterInfo &MRI,
  MachineIRBuilder &B) const {
  B.setInstr(MI);

  const LLT S1 = LLT::scalar(1);
  const LLT S32 = LLT::scalar(32);
  const LLT S64 = LLT::scalar(64);

  Register Src = MI.getOperand(1).getReg();
  assert(MRI.getType(Src) == S64);

  // TODO: Should this use extract since the low half is unused?
  auto Unmerge = B.buildUnmerge({S32, S32}, Src);
  Register Hi = Unmerge.getReg(1);

  // Extract the upper half, since this is where we will find the sign and
  // exponent.
  auto Exp = extractF64Exponent(Hi, B);

  const unsigned FractBits = 52;

  // Extract the sign bit.
  const auto SignBitMask = B.buildConstant(S32, UINT32_C(1) << 31);
  auto SignBit = B.buildAnd(S32, Hi, SignBitMask);

  const auto FractMask = B.buildConstant(S64, (UINT64_C(1) << FractBits) - 1);

  const auto Zero32 = B.buildConstant(S32, 0);

  // Extend back to 64-bits.
  auto SignBit64 = B.buildMerge(S64, {Zero32.getReg(0), SignBit.getReg(0)});

  auto Shr = B.buildAShr(S64, FractMask, Exp);
  auto Not = B.buildNot(S64, Shr);
  auto Tmp0 = B.buildAnd(S64, Src, Not);
  auto FiftyOne = B.buildConstant(S32, FractBits - 1);

  auto ExpLt0 = B.buildICmp(CmpInst::ICMP_SLT, S1, Exp, Zero32);
  auto ExpGt51 = B.buildICmp(CmpInst::ICMP_SGT, S1, Exp, FiftyOne);

  auto Tmp1 = B.buildSelect(S64, ExpLt0, SignBit64, Tmp0);
  B.buildSelect(MI.getOperand(0).getReg(), ExpGt51, Src, Tmp1);
  return true;
}

bool AMDGPULegalizerInfo::legalizeITOFP(
  MachineInstr &MI, MachineRegisterInfo &MRI,
  MachineIRBuilder &B, bool Signed) const {
  B.setInstr(MI);

  Register Dst = MI.getOperand(0).getReg();
  Register Src = MI.getOperand(1).getReg();

  const LLT S64 = LLT::scalar(64);
  const LLT S32 = LLT::scalar(32);

  assert(MRI.getType(Src) == S64 && MRI.getType(Dst) == S64);

  auto Unmerge = B.buildUnmerge({S32, S32}, Src);

  auto CvtHi = Signed ?
    B.buildSITOFP(S64, Unmerge.getReg(1)) :
    B.buildUITOFP(S64, Unmerge.getReg(1));

  auto CvtLo = B.buildUITOFP(S64, Unmerge.getReg(0));

  auto ThirtyTwo = B.buildConstant(S32, 32);
  auto LdExp = B.buildIntrinsic(Intrinsic::amdgcn_ldexp, {S64}, false)
    .addUse(CvtHi.getReg(0))
    .addUse(ThirtyTwo.getReg(0));

  // TODO: Should this propagate fast-math-flags?
  B.buildFAdd(Dst, LdExp, CvtLo);
  MI.eraseFromParent();
  return true;
}

// Return the use branch instruction, otherwise null if the usage is invalid.
static MachineInstr *verifyCFIntrinsic(MachineInstr &MI,
                                       MachineRegisterInfo &MRI) {
  Register CondDef = MI.getOperand(0).getReg();
  if (!MRI.hasOneNonDBGUse(CondDef))
    return nullptr;

  MachineInstr &UseMI = *MRI.use_instr_nodbg_begin(CondDef);
  return UseMI.getParent() == MI.getParent() &&
    UseMI.getOpcode() == AMDGPU::G_BRCOND ? &UseMI : nullptr;
}

bool AMDGPULegalizerInfo::legalizeIntrinsic(MachineInstr &MI,
                                            MachineRegisterInfo &MRI,
                                            MachineIRBuilder &B) const {
  // Replace the use G_BRCOND with the exec manipulate and branch pseudos.
  switch (MI.getOperand(MI.getNumExplicitDefs()).getIntrinsicID()) {
  case Intrinsic::amdgcn_if: {
    if (MachineInstr *BrCond = verifyCFIntrinsic(MI, MRI)) {
      const SIRegisterInfo *TRI
        = static_cast<const SIRegisterInfo *>(MRI.getTargetRegisterInfo());

      B.setInstr(*BrCond);
      Register Def = MI.getOperand(1).getReg();
      Register Use = MI.getOperand(3).getReg();
      B.buildInstr(AMDGPU::SI_IF)
        .addDef(Def)
        .addUse(Use)
        .addMBB(BrCond->getOperand(1).getMBB());

      MRI.setRegClass(Def, TRI->getWaveMaskRegClass());
      MRI.setRegClass(Use, TRI->getWaveMaskRegClass());
      MI.eraseFromParent();
      BrCond->eraseFromParent();
      return true;
    }

    return false;
  }
  case Intrinsic::amdgcn_loop: {
    if (MachineInstr *BrCond = verifyCFIntrinsic(MI, MRI)) {
      const SIRegisterInfo *TRI
        = static_cast<const SIRegisterInfo *>(MRI.getTargetRegisterInfo());

      B.setInstr(*BrCond);
      Register Reg = MI.getOperand(2).getReg();
      B.buildInstr(AMDGPU::SI_LOOP)
        .addUse(Reg)
        .addMBB(BrCond->getOperand(1).getMBB());
      MI.eraseFromParent();
      BrCond->eraseFromParent();
      MRI.setRegClass(Reg, TRI->getWaveMaskRegClass());
      return true;
    }

    return false;
  }
  default:
    return true;
  }

  return true;
}
