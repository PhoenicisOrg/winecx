@ stdcall -norelay ExAcquireFastMutexUnsafe(ptr)
@ stub ExAcquireRundownProtection
@ stub ExAcquireRundownProtectionEx
@ stub ExInitializeRundownProtection
@ stub ExInterlockedAddLargeStatistic
@ stub ExInterlockedCompareExchange64
@ stub ExInterlockedFlushSList
@ stdcall -norelay ExInterlockedPopEntrySList(ptr ptr) NTOSKRNL_ExInterlockedPopEntrySList
@ stdcall -norelay ExInterlockedPushEntrySList (ptr ptr ptr) NTOSKRNL_ExInterlockedPushEntrySList
@ stub ExReInitializeRundownProtection
@ stdcall -norelay ExReleaseFastMutexUnsafe(ptr)
@ stdcall ExReleaseResourceLite(ptr)
@ stub ExReleaseRundownProtection
@ stub ExReleaseRundownProtectionEx
@ stub ExRundownCompleted
@ stub ExWaitForRundownProtectionRelease
@ stub ExfAcquirePushLockExclusive
@ stub ExfAcquirePushLockShared
@ stub ExfInterlockedAddUlong
@ stub ExfInterlockedCompareExchange64
@ stub ExfInterlockedInsertHeadList
@ stub ExfInterlockedInsertTailList
@ stub ExfInterlockedPopEntryList
@ stub ExfInterlockedPushEntryList
@ stdcall -norelay ExfInterlockedRemoveHeadList(ptr ptr)
@ stub ExfReleasePushLock
@ stub Exfi386InterlockedDecrementLong
@ stub Exfi386InterlockedExchangeUlong
@ stub Exfi386InterlockedIncrementLong
@ stub HalExamineMBR
@ stdcall -norelay InterlockedCompareExchange(ptr long long) NTOSKRNL_InterlockedCompareExchange
@ stdcall -norelay InterlockedDecrement(ptr) NTOSKRNL_InterlockedDecrement
@ stdcall -norelay InterlockedExchange(ptr long) NTOSKRNL_InterlockedExchange
@ stdcall -norelay InterlockedExchangeAdd(ptr long) NTOSKRNL_InterlockedExchangeAdd
@ stdcall -norelay InterlockedIncrement(ptr) NTOSKRNL_InterlockedIncrement
@ stdcall -norelay InterlockedPopEntrySList(ptr) NTOSKRNL_InterlockedPopEntrySList
@ stdcall -norelay InterlockedPushEntrySList(ptr ptr) NTOSKRNL_InterlockedPushEntrySList
@ stub IoAssignDriveLetters
@ stub IoReadPartitionTable
@ stub IoSetPartitionInformation
@ stub IoWritePartitionTable
@ stdcall -norelay IofCallDriver(ptr ptr)
@ stdcall -norelay IofCompleteRequest(ptr long)
@ stdcall -norelay KeAcquireInStackQueuedSpinLock(ptr ptr)
@ stub KeAcquireInStackQueuedSpinLockAtDpcLevel
@ stdcall -norelay KeReleaseInStackQueuedSpinLock(ptr)
@ stub KeReleaseInStackQueuedSpinLockFromDpcLevel
@ stub KeSetTimeUpdateNotifyRoutine
@ stub KefAcquireSpinLockAtDpcLevel
@ stub KefReleaseSpinLockFromDpcLevel
@ stub KiAcquireSpinLock
@ stub KiReleaseSpinLock
@ stdcall -norelay ObfDereferenceObject(ptr)
@ stdcall -norelay ObfReferenceObject(ptr)
@ stub RtlPrefetchMemoryNonTemporal
@ cdecl -i386 -norelay RtlUlongByteSwap()
@ cdecl -ret64 RtlUlonglongByteSwap(int64)
@ cdecl -i386 -norelay RtlUshortByteSwap()
@ stub WmiGetClock
@ stub Kei386EoiHelper
@ stub Kii386SpinOnSpinLock
@ stub CcCanIWrite
@ stub CcCopyRead
@ stub CcCopyWrite
@ stub CcDeferWrite
@ stub CcFastCopyRead
@ stub CcFastCopyWrite
@ stub CcFastMdlReadWait
@ stub CcFastReadNotPossible
@ stub CcFastReadWait
@ stub CcFlushCache
@ stub CcGetDirtyPages
@ stub CcGetFileObjectFromBcb
@ stub CcGetFileObjectFromSectionPtrs
@ stub CcGetFlushedValidData
@ stub CcGetLsnForFileObject
@ stub CcInitializeCacheMap
@ stub CcIsThereDirtyData
@ stub CcMapData
@ stub CcMdlRead
@ stub CcMdlReadComplete
@ stub CcMdlWriteAbort
@ stub CcMdlWriteComplete
@ stub CcPinMappedData
@ stub CcPinRead
@ stub CcPrepareMdlWrite
@ stub CcPreparePinWrite
@ stub CcPurgeCacheSection
@ stub CcRemapBcb
@ stub CcRepinBcb
@ stub CcScheduleReadAhead
@ stub CcSetAdditionalCacheAttributes
@ stub CcSetBcbOwnerPointer
@ stub CcSetDirtyPageThreshold
@ stub CcSetDirtyPinnedData
@ stub CcSetFileSizes
@ stub CcSetLogHandleForFile
@ stub CcSetReadAheadGranularity
@ stub CcUninitializeCacheMap
@ stub CcUnpinData
@ stub CcUnpinDataForThread
@ stub CcUnpinRepinnedBcb
@ stub CcWaitForCurrentLazyWriterActivity
@ stub CcZeroData
@ stdcall CmRegisterCallback(ptr ptr ptr)
@ stdcall CmUnRegisterCallback(int64)
@ stdcall DbgBreakPoint()
@ stub DbgBreakPointWithStatus
@ stub DbgLoadImageSymbols
@ varargs DbgPrint(str)
@ varargs DbgPrintEx(long long str)
@ stub DbgPrintReturnControlC
@ stub DbgPrompt
@ stdcall DbgQueryDebugFilterState(long long)
@ stub DbgSetDebugFilterState
@ stdcall ExAcquireResourceExclusiveLite(ptr long)
@ stub ExAcquireResourceSharedLite
@ stub ExAcquireSharedStarveExclusive
@ stub ExAcquireSharedWaitForExclusive
@ stub ExAllocateFromPagedLookasideList
@ stdcall ExAllocatePool(long long)
@ stdcall ExAllocatePoolWithQuota(long long)
@ stdcall ExAllocatePoolWithQuotaTag(long long long)
@ stdcall ExAllocatePoolWithTag(long long long)
@ stub ExAllocatePoolWithTagPriority
@ stub ExConvertExclusiveToSharedLite
@ stdcall ExCreateCallback(ptr ptr long long)
@ stdcall ExDeleteNPagedLookasideList(ptr)
@ stdcall ExDeletePagedLookasideList(ptr)
@ stdcall ExDeleteResourceLite(ptr)
@ stub ExDesktopObjectType
@ stub ExDisableResourceBoostLite
@ stub ExEnumHandleTable
@ stub ExEventObjectType
@ stub ExExtendZone
@ stdcall -norelay ExfUnblockPushLock(ptr ptr)
@ stdcall ExFreePool(ptr)
@ stdcall ExFreePoolWithTag(ptr long)
@ stub ExFreeToPagedLookasideList
@ stub ExGetCurrentProcessorCounts
@ stub ExGetCurrentProcessorCpuUsage
@ stub ExGetExclusiveWaiterCount
@ stub ExGetPreviousMode
@ stub ExGetSharedWaiterCount
@ stdcall ExInitializeNPagedLookasideList(ptr ptr ptr long long long long)
@ stdcall ExInitializePagedLookasideList(ptr ptr ptr long long long long)
@ stdcall ExInitializeResourceLite(ptr)
@ stdcall ExInitializeZone(ptr long ptr long)
@ stub ExInterlockedAddLargeInteger
@ stub ExInterlockedAddUlong
@ stub ExInterlockedDecrementLong
@ stub ExInterlockedExchangeUlong
@ stub ExInterlockedExtendZone
@ stub ExInterlockedIncrementLong
@ stub ExInterlockedInsertHeadList
@ stub ExInterlockedInsertTailList
@ stub ExInterlockedPopEntryList
@ stub ExInterlockedPushEntryList
@ stdcall ExInterlockedRemoveHeadList(ptr ptr)
@ stub ExIsProcessorFeaturePresent
@ stub ExIsResourceAcquiredExclusiveLite
@ stub ExIsResourceAcquiredSharedLite
@ stdcall ExLocalTimeToSystemTime(ptr ptr) RtlLocalTimeToSystemTime
@ stub ExNotifyCallback
@ stub ExQueryPoolBlockSize
@ stub ExQueueWorkItem
@ stub ExRaiseAccessViolation
@ stub ExRaiseDatatypeMisalignment
@ stub ExRaiseException
@ stub ExRaiseHardError
@ stub ExRaiseStatus
@ stub ExRegisterCallback
@ stub ExReinitializeResourceLite
@ stdcall ExReleaseResourceForThreadLite(ptr long)
@ stub ExSemaphoreObjectType
@ stub ExSetResourceOwnerPointer
@ stub ExSetTimerResolution
@ stub ExSystemExceptionFilter
@ stdcall ExSystemTimeToLocalTime(ptr ptr) RtlSystemTimeToLocalTime
@ stub ExUnregisterCallback
@ stub ExUuidCreate
@ stub ExVerifySuite
@ stub ExWindowStationObjectType
@ stub Exi386InterlockedDecrementLong
@ stub Exi386InterlockedExchangeUlong
@ stub Exi386InterlockedIncrementLong
@ stub FsRtlAcquireFileExclusive
@ stub FsRtlAddLargeMcbEntry
@ stub FsRtlAddMcbEntry
@ stub FsRtlAddToTunnelCache
@ stub FsRtlAllocateFileLock
@ stub FsRtlAllocatePool
@ stub FsRtlAllocatePoolWithQuota
@ stub FsRtlAllocatePoolWithQuotaTag
@ stub FsRtlAllocatePoolWithTag
@ stub FsRtlAllocateResource
@ stub FsRtlAreNamesEqual
@ stub FsRtlBalanceReads
@ stub FsRtlCheckLockForReadAccess
@ stub FsRtlCheckLockForWriteAccess
@ stub FsRtlCheckOplock
@ stub FsRtlCopyRead
@ stub FsRtlCopyWrite
@ stub FsRtlCurrentBatchOplock
@ stub FsRtlDeleteKeyFromTunnelCache
@ stub FsRtlDeleteTunnelCache
@ stub FsRtlDeregisterUncProvider
@ stub FsRtlDissectDbcs
@ stub FsRtlDissectName
@ stub FsRtlDoesDbcsContainWildCards
@ stub FsRtlDoesNameContainWildCards
@ stub FsRtlFastCheckLockForRead
@ stub FsRtlFastCheckLockForWrite
@ stub FsRtlFastUnlockAll
@ stub FsRtlFastUnlockAllByKey
@ stub FsRtlFastUnlockSingle
@ stub FsRtlFindInTunnelCache
@ stub FsRtlFreeFileLock
@ stub FsRtlGetFileSize
@ stub FsRtlGetNextFileLock
@ stub FsRtlGetNextLargeMcbEntry
@ stub FsRtlGetNextMcbEntry
@ stub FsRtlIncrementCcFastReadNoWait
@ stub FsRtlIncrementCcFastReadNotPossible
@ stub FsRtlIncrementCcFastReadResourceMiss
@ stub FsRtlIncrementCcFastReadWait
@ stub FsRtlInitializeFileLock
@ stub FsRtlInitializeLargeMcb
@ stub FsRtlInitializeMcb
@ stub FsRtlInitializeOplock
@ stub FsRtlInitializeTunnelCache
@ stub FsRtlInsertPerFileObjectContext
@ stub FsRtlInsertPerStreamContext
@ stub FsRtlIsDbcsInExpression
@ stub FsRtlIsFatDbcsLegal
@ stub FsRtlIsHpfsDbcsLegal
@ stdcall FsRtlIsNameInExpression(ptr ptr long ptr)
@ stub FsRtlIsNtstatusExpected
@ stub FsRtlIsPagingFile
@ stub FsRtlIsTotalDeviceFailure
@ stub FsRtlLegalAnsiCharacterArray
@ stub FsRtlLookupLargeMcbEntry
@ stub FsRtlLookupLastLargeMcbEntry
@ stub FsRtlLookupLastLargeMcbEntryAndIndex
@ stub FsRtlLookupLastMcbEntry
@ stub FsRtlLookupMcbEntry
@ stub FsRtlLookupPerFileObjectContext
@ stub FsRtlLookupPerStreamContextInternal
@ stub FsRtlMdlRead
@ stub FsRtlMdlReadComplete
@ stub FsRtlMdlReadCompleteDev
@ stub FsRtlMdlReadDev
@ stub FsRtlMdlWriteComplete
@ stub FsRtlMdlWriteCompleteDev
@ stub FsRtlNormalizeNtstatus
@ stub FsRtlNotifyChangeDirectory
@ stub FsRtlNotifyCleanup
@ stub FsRtlNotifyFilterChangeDirectory
@ stub FsRtlNotifyFilterReportChange
@ stub FsRtlNotifyFullChangeDirectory
@ stub FsRtlNotifyFullReportChange
@ stub FsRtlNotifyInitializeSync
@ stub FsRtlNotifyReportChange
@ stub FsRtlNotifyUninitializeSync
@ stub FsRtlNotifyVolumeEvent
@ stub FsRtlNumberOfRunsInLargeMcb
@ stub FsRtlNumberOfRunsInMcb
@ stub FsRtlOplockFsctrl
@ stub FsRtlOplockIsFastIoPossible
@ stub FsRtlPostPagingFileStackOverflow
@ stub FsRtlPostStackOverflow
@ stub FsRtlPrepareMdlWrite
@ stub FsRtlPrepareMdlWriteDev
@ stub FsRtlPrivateLock
@ stub FsRtlProcessFileLock
@ stdcall FsRtlRegisterFileSystemFilterCallbacks(ptr ptr)
@ stdcall FsRtlRegisterUncProvider(ptr ptr long)
@ stub FsRtlReleaseFile
@ stub FsRtlRemoveLargeMcbEntry
@ stub FsRtlRemoveMcbEntry
@ stub FsRtlRemovePerFileObjectContext
@ stub FsRtlRemovePerStreamContext
@ stub FsRtlResetLargeMcb
@ stub FsRtlSplitLargeMcb
@ stub FsRtlSyncVolumes
@ stub FsRtlTeardownPerStreamContexts
@ stub FsRtlTruncateLargeMcb
@ stub FsRtlTruncateMcb
@ stub FsRtlUninitializeFileLock
@ stub FsRtlUninitializeLargeMcb
@ stub FsRtlUninitializeMcb
@ stub FsRtlUninitializeOplock
@ stub HalDispatchTable
@ stub HalPrivateDispatchTable
@ stub HeadlessDispatch
@ stub InbvAcquireDisplayOwnership
@ stub InbvCheckDisplayOwnership
@ stub InbvDisplayString
@ stub InbvEnableBootDriver
@ stub InbvEnableDisplayString
@ stub InbvInstallDisplayStringFilter
@ stub InbvIsBootDriverInstalled
@ stub InbvNotifyDisplayOwnershipLost
@ stub InbvResetDisplay
@ stub InbvSetScrollRegion
@ stub InbvSetTextColor
@ stub InbvSolidColorFill
@ extern InitSafeBootMode
@ stdcall IoAcquireCancelSpinLock(ptr)
@ stdcall IoAcquireRemoveLockEx(ptr ptr str long long)
@ stub IoAcquireVpbSpinLock
@ stub IoAdapterObjectType
@ stub IoAllocateAdapterChannel
@ stub IoAllocateController
@ stdcall IoAllocateDriverObjectExtension(ptr ptr long ptr)
@ stdcall IoAllocateErrorLogEntry(ptr long)
@ stdcall IoAllocateIrp(long long)
@ stdcall IoAllocateMdl(ptr long long long ptr)
@ stdcall IoAllocateWorkItem(ptr)
@ stub IoAssignResources
@ stdcall IoAttachDevice(ptr ptr ptr)
@ stub IoAttachDeviceByPointer
@ stdcall IoAttachDeviceToDeviceStack(ptr ptr)
@ stub IoAttachDeviceToDeviceStackSafe
@ stub IoBuildAsynchronousFsdRequest
@ stdcall IoBuildDeviceIoControlRequest(long ptr ptr long ptr long long ptr ptr)
@ stub IoBuildPartialMdl
@ stdcall IoBuildSynchronousFsdRequest(long ptr ptr long ptr ptr ptr)
@ stdcall IoCallDriver(ptr ptr)
@ stub IoCancelFileOpen
@ stub IoCancelIrp
@ stub IoCheckDesiredAccess
@ stub IoCheckEaBufferValidity
@ stub IoCheckFunctionAccess
@ stub IoCheckQuerySetFileInformation
@ stub IoCheckQuerySetVolumeInformation
@ stub IoCheckQuotaBufferValidity
@ stub IoCheckShareAccess
@ stdcall IoCompleteRequest(ptr long)
@ stub IoConnectInterrupt
@ stub IoCreateController
@ stdcall IoCreateDevice(ptr long ptr long long long ptr)
@ stub IoCreateDisk
@ stdcall IoCreateDriver(ptr ptr)
@ stdcall IoCreateFile(ptr long ptr ptr ptr long long long long ptr long long ptr long)
@ stub IoCreateFileSpecifyDeviceObjectHint
@ stdcall IoCreateNotificationEvent(ptr ptr)
@ stub IoCreateStreamFileObject
@ stub IoCreateStreamFileObjectEx
@ stub IoCreateStreamFileObjectLite
@ stdcall IoCreateSymbolicLink(ptr ptr)
@ stdcall IoCreateSynchronizationEvent(ptr ptr)
@ stub IoCreateUnprotectedSymbolicLink
@ stdcall IoCsqInitialize(ptr ptr ptr ptr ptr ptr ptr)
@ stub IoCsqInsertIrp
@ stub IoCsqRemoveIrp
@ stub IoCsqRemoveNextIrp
@ stub IoDeleteController
@ stdcall IoDeleteDevice(ptr)
@ stdcall IoDeleteDriver(ptr)
@ stdcall IoDeleteSymbolicLink(ptr)
@ stub IoDetachDevice
@ stub IoDeviceHandlerObjectSize
@ stub IoDeviceHandlerObjectType
@ stub IoDeviceObjectType
@ stub IoDisconnectInterrupt
@ stub IoDriverObjectType
@ stub IoEnqueueIrp
@ stub IoEnumerateDeviceObjectList
@ stub IoFastQueryNetworkAttributes
@ stub IoFileObjectType
@ stub IoForwardAndCatchIrp
@ stub IoForwardIrpSynchronously
@ stub IoFreeController
@ stub IoFreeErrorLogEntry
@ stdcall IoFreeIrp(ptr)
@ stdcall IoFreeMdl(ptr)
@ stub IoFreeWorkItem
@ stdcall IoGetAttachedDevice(ptr)
@ stdcall IoGetAttachedDeviceReference(ptr)
@ stub IoGetBaseFileSystemDeviceObject
@ stub IoGetBootDiskInformation
@ stdcall IoGetConfigurationInformation()
@ stdcall IoGetCurrentProcess()
@ stub IoGetDeviceAttachmentBaseRef
@ stub IoGetDeviceInterfaceAlias
@ stdcall IoGetDeviceInterfaces(ptr ptr long ptr)
@ stdcall IoGetDeviceObjectPointer(ptr long ptr ptr)
@ stdcall IoGetDeviceProperty(ptr long long ptr ptr)
@ stub IoGetDeviceToVerify
@ stub IoGetDiskDeviceObject
@ stub IoGetDmaAdapter
@ stdcall IoGetDriverObjectExtension(ptr ptr)
@ stub IoGetFileObjectGenericMapping
@ stub IoGetInitialStack
@ stub IoGetLowerDeviceObject
@ stdcall IoGetRelatedDeviceObject(ptr)
@ stub IoGetRequestorProcess
@ stub IoGetRequestorProcessId
@ stub IoGetRequestorSessionId
@ stub IoGetStackLimits
@ stub IoGetTopLevelIrp
@ stdcall IoInitializeIrp(ptr long long)
@ stdcall IoInitializeRemoveLockEx(ptr long long long long)
@ stdcall IoInitializeTimer(ptr ptr ptr)
@ stdcall IoInvalidateDeviceRelations(ptr long)
@ stub IoInvalidateDeviceState
@ stub IoIsFileOriginRemote
@ stub IoIsOperationSynchronous
@ stub IoIsSystemThread
@ stub IoIsValidNameGraftingBuffer
@ stdcall IoIsWdmVersionAvailable(long long)
@ stub IoMakeAssociatedIrp
@ stub IoOpenDeviceInterfaceRegistryKey
@ stub IoOpenDeviceRegistryKey
@ stub IoPageRead
@ stub IoPnPDeliverServicePowerNotification
@ stdcall IoQueryDeviceDescription(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub IoQueryFileDosDeviceName
@ stub IoQueryFileInformation
@ stub IoQueryVolumeInformation
@ stub IoQueueThreadIrp
@ stub IoQueueWorkItem
@ stub IoRaiseHardError
@ stub IoRaiseInformationalHardError
@ stub IoReadDiskSignature
@ stub IoReadOperationCount
@ stub IoReadPartitionTableEx
@ stub IoReadTransferCount
@ stub IoRegisterBootDriverReinitialization
@ stdcall IoRegisterDeviceInterface(ptr ptr ptr ptr)
@ stdcall IoRegisterDriverReinitialization(ptr ptr ptr)
@ stdcall IoRegisterFileSystem(ptr)
@ stub IoRegisterFsRegistrationChange
@ stub IoRegisterLastChanceShutdownNotification
@ stdcall IoRegisterPlugPlayNotification(long long ptr ptr ptr ptr ptr)
@ stdcall IoRegisterShutdownNotification(ptr)
@ stdcall IoReleaseCancelSpinLock(long)
@ stdcall IoReleaseRemoveLockAndWaitEx(ptr ptr long)
@ stub IoReleaseRemoveLockEx
@ stub IoReleaseVpbSpinLock
@ stub IoRemoveShareAccess
@ stub IoReportDetectedDevice
@ stub IoReportHalResourceUsage
@ stdcall IoReportResourceForDetection(ptr ptr long ptr ptr long ptr)
@ stdcall IoReportResourceUsage(ptr ptr ptr long ptr ptr long long ptr)
@ stub IoReportTargetDeviceChange
@ stub IoReportTargetDeviceChangeAsynchronous
@ stub IoRequestDeviceEject
@ stub IoReuseIrp
@ stub IoSetCompletionRoutineEx
@ stdcall IoSetDeviceInterfaceState(ptr long)
@ stub IoSetDeviceToVerify
@ stub IoSetFileOrigin
@ stub IoSetHardErrorOrVerifyDevice
@ stub IoSetInformation
@ stub IoSetIoCompletion
@ stub IoSetPartitionInformationEx
@ stub IoSetShareAccess
@ stub IoSetStartIoAttributes
@ stub IoSetSystemPartition
@ stdcall IoSetThreadHardErrorMode(long)
@ stub IoSetTopLevelIrp
@ stdcall IoStartNextPacket(ptr long)
@ stub IoStartNextPacketByKey
@ stub IoStartPacket
@ stdcall IoStartTimer(ptr)
@ stub IoStatisticsLock
@ stdcall IoStopTimer(ptr)
@ stub IoSynchronousInvalidateDeviceRelations
@ stub IoSynchronousPageWrite
@ stub IoThreadToProcess
@ stdcall IoUnregisterFileSystem(ptr)
@ stub IoUnregisterFsRegistrationChange
@ stdcall IoUnregisterPlugPlayNotification(ptr)
@ stdcall IoUnregisterShutdownNotification(ptr)
@ stub IoUpdateShareAccess
@ stub IoValidateDeviceIoControlAccess
@ stub IoVerifyPartitionTable
@ stub IoVerifyVolume
@ stub IoVolumeDeviceToDosName
@ stub IoWMIAllocateInstanceIds
@ stub IoWMIDeviceObjectToInstanceName
@ stub IoWMIExecuteMethod
@ stub IoWMIHandleToInstanceName
@ stub IoWMIOpenBlock
@ stub IoWMIQueryAllData
@ stub IoWMIQueryAllDataMultiple
@ stub IoWMIQuerySingleInstance
@ stub IoWMIQuerySingleInstanceMultiple
@ stdcall IoWMIRegistrationControl(ptr long)
@ stub IoWMISetNotificationCallback
@ stub IoWMISetSingleInstance
@ stub IoWMISetSingleItem
@ stub IoWMISuggestInstanceName
@ stub IoWMIWriteEvent
@ stub IoWriteErrorLogEntry
@ stub IoWriteOperationCount
@ stub IoWritePartitionTableEx
@ stub IoWriteTransferCount
@ extern KdDebuggerEnabled
@ stub KdDebuggerNotPresent
@ stub KdDisableDebugger
@ stub KdEnableDebugger
@ stub KdEnteredDebugger
@ stub KdPollBreakIn
@ stub KdPowerTransition
@ stub Ke386CallBios
@ stdcall Ke386IoSetAccessProcess(ptr long)
@ stub Ke386QueryIoAccessMap
@ stdcall Ke386SetIoAccessMap(long ptr)
@ stub KeAcquireInterruptSpinLock
@ stub KeAcquireSpinLockAtDpcLevel
@ stdcall -arch=x86_64 KeAcquireSpinLockRaiseToDpc(ptr)
@ stub KeAddSystemServiceTable
@ stub KeAreApcsDisabled
@ stub KeAttachProcess
@ stub KeBugCheck
@ stub KeBugCheckEx
@ stdcall KeCancelTimer(ptr)
@ stub KeCapturePersistentThreadState
@ stdcall KeClearEvent(ptr)
@ stub KeConnectInterrupt
@ stub KeDcacheFlushCount
@ stdcall KeDelayExecutionThread(long long ptr)
@ stub KeDeregisterBugCheckCallback
@ stub KeDeregisterBugCheckReasonCallback
@ stub KeDetachProcess
@ stub KeDisconnectInterrupt
@ stdcall KeEnterCriticalRegion()
@ stub KeEnterKernelDebugger
@ stub KeFindConfigurationEntry
@ stub KeFindConfigurationNextEntry
@ stub KeFlushEntireTb
@ stdcall KeFlushQueuedDpcs()
@ stdcall KeGetCurrentThread()
@ stub KeGetPreviousMode
@ stub KeGetRecommendedSharedDataAlignment
@ stub KeI386AbiosCall
@ stub KeI386AllocateGdtSelectors
@ stub KeI386Call16BitCStyleFunction
@ stub KeI386Call16BitFunction
@ stub KeI386FlatToGdtSelector
@ stub KeI386GetLid
@ stub KeI386MachineType
@ stub KeI386ReleaseGdtSelectors
@ stub KeI386ReleaseLid
@ stub KeI386SetGdtSelector
@ stub KeIcacheFlushCount
@ stub KeInitializeApc
@ stub KeInitializeDeviceQueue
@ stdcall KeInitializeDpc(ptr ptr ptr)
@ stdcall KeInitializeEvent(ptr long long)
@ stub KeInitializeInterrupt
@ stub KeInitializeMutant
@ stdcall KeInitializeMutex(ptr long)
@ stub KeInitializeQueue
@ stdcall KeInitializeSemaphore(ptr long long)
@ stdcall KeInitializeSpinLock(ptr)
@ stdcall KeInitializeTimer(ptr)
@ stdcall KeInitializeTimerEx(ptr long)
@ stub KeInsertByKeyDeviceQueue
@ stub KeInsertDeviceQueue
@ stub KeInsertHeadQueue
@ stdcall KeInsertQueue(ptr ptr)
@ stub KeInsertQueueApc
@ stub KeInsertQueueDpc
@ stub KeIsAttachedProcess
@ stub KeIsExecutingDpc
@ stdcall KeLeaveCriticalRegion()
@ stub KeLoaderBlock
@ stub KeNumberProcessors
@ stub KeProfileInterrupt
@ stub KeProfileInterruptWithSource
@ stub KePulseEvent
@ stdcall KeQueryActiveProcessors()
@ stdcall KeQueryInterruptTime()
@ stub KeQueryPriorityThread
@ stub KeQueryRuntimeThread
@ stdcall KeQuerySystemTime(ptr)
@ stdcall KeQueryTickCount(ptr)
@ stdcall KeQueryTimeIncrement()
@ stub KeRaiseUserException
@ stub KeReadStateEvent
@ stub KeReadStateMutant
@ stub KeReadStateMutex
@ stub KeReadStateQueue
@ stub KeReadStateSemaphore
@ stub KeReadStateTimer
@ stub KeRegisterBugCheckCallback
@ stub KeRegisterBugCheckReasonCallback
@ stub KeReleaseInterruptSpinLock
@ stub KeReleaseMutant
@ stdcall KeReleaseMutex(ptr long)
@ stdcall KeReleaseSemaphore(ptr long long long)
@ stdcall KeReleaseSpinLock(ptr long)
@ stub KeReleaseSpinLockFromDpcLevel
@ stub KeRemoveByKeyDeviceQueue
@ stub KeRemoveByKeyDeviceQueueIfBusy
@ stub KeRemoveDeviceQueue
@ stub KeRemoveEntryDeviceQueue
@ stub KeRemoveQueue
@ stub KeRemoveQueueDpc
@ stub KeRemoveSystemServiceTable
@ stdcall KeResetEvent(ptr)
@ stub KeRestoreFloatingPointState
@ stub KeRevertToUserAffinityThread
@ stub KeRundownQueue
@ stub KeSaveFloatingPointState
@ stub KeSaveStateForHibernate
@ extern KeServiceDescriptorTable
@ stub KeSetAffinityThread
@ stub KeSetBasePriorityThread
@ stub KeSetDmaIoCoherency
@ stdcall KeSetEvent(ptr long long)
@ stub KeSetEventBoostPriority
@ stub KeSetIdealProcessorThread
@ stub KeSetImportanceDpc
@ stub KeSetKernelStackSwapEnable
@ stdcall KeSetPriorityThread(ptr long)
@ stub KeSetProfileIrql
@ stdcall KeSetSystemAffinityThread(long)
@ stdcall KeSetTargetProcessorDpc(ptr long)
@ stub KeSetTimeIncrement
@ stub KeSetTimer
@ stdcall KeSetTimerEx(ptr int64 long ptr)
@ stub KeStackAttachProcess
@ stub KeSynchronizeExecution
@ stub KeTerminateThread
@ extern KeTickCount
@ stub KeUnstackDetachProcess
@ stub KeUpdateRunTime
@ stub KeUpdateSystemTime
@ stub KeUserModeCallback
@ stdcall KeWaitForMultipleObjects(long ptr long long long long ptr ptr)
@ stdcall KeWaitForMutexObject(ptr long long long ptr)
@ stdcall KeWaitForSingleObject(ptr long long long ptr)
@ stub KiBugCheckData
@ stub KiCoprocessorError
@ stub KiDeliverApc
@ stub KiDispatchInterrupt
@ stub KiEnableTimerWatchdog
@ stub KiIpiServiceRoutine
@ stub KiUnexpectedInterrupt
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stub LdrEnumResources
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
@ stdcall LdrFindResource_U(long ptr long ptr)
@ stub LpcPortObjectType
@ stub LpcRequestPort
@ stub LpcRequestWaitReplyPort
@ stub LsaCallAuthenticationPackage
@ stub LsaDeregisterLogonProcess
@ stub LsaFreeReturnBuffer
@ stub LsaLogonUser
@ stub LsaLookupAuthenticationPackage
@ stub LsaRegisterLogonProcess
@ stub Mm64BitPhysicalAddress
@ stub MmAddPhysicalMemory
@ stub MmAddVerifierThunks
@ stub MmAdjustWorkingSetSize
@ stub MmAdvanceMdl
@ stdcall MmAllocateContiguousMemory(long int64)
@ stdcall MmAllocateContiguousMemorySpecifyCache(long int64 int64 int64 long)
@ stub MmAllocateMappingAddress
@ stdcall MmAllocateNonCachedMemory(long)
@ stdcall MmAllocatePagesForMdl(int64 int64 int64 long)
@ stub MmBuildMdlForNonPagedPool
@ stub MmCanFileBeTruncated
@ stub MmCommitSessionMappedView
@ stdcall MmCopyVirtualMemory(ptr ptr ptr ptr long long ptr)
@ stub MmCreateMdl
@ stdcall MmCreateSection(ptr long ptr ptr long long long ptr)
@ stub MmDisableModifiedWriteOfSection
@ stub MmFlushImageSection
@ stub MmForceSectionClosed
@ stub MmFreeContiguousMemory
@ stub MmFreeContiguousMemorySpecifyCache
@ stub MmFreeMappingAddress
@ stdcall MmFreeNonCachedMemory(ptr long)
@ stub MmFreePagesFromMdl
@ stub MmGetPhysicalAddress
@ stub MmGetPhysicalMemoryRanges
@ stdcall MmGetSystemRoutineAddress(ptr)
@ stub MmGetVirtualForPhysical
@ stub MmGrowKernelStack
@ stub MmHighestUserAddress
@ stdcall MmIsAddressValid(ptr)
@ stub MmIsDriverVerifying
@ stub MmIsNonPagedSystemAddressValid
@ stub MmIsRecursiveIoFault
@ stub MmIsThisAnNtAsSystem
@ stub MmIsVerifierEnabled
@ stub MmLockPagableDataSection
@ stub MmLockPagableImageSection
@ stdcall MmLockPagableSectionByHandle(ptr)
@ stdcall MmMapIoSpace(int64 long long)
@ stub MmMapLockedPages
@ stdcall MmMapLockedPagesSpecifyCache(ptr long long ptr long long)
@ stub MmMapLockedPagesWithReservedMapping
@ stub MmMapMemoryDumpMdl
@ stub MmMapUserAddressesToPage
@ stub MmMapVideoDisplay
@ stub MmMapViewInSessionSpace
@ stub MmMapViewInSystemSpace
@ stub MmMapViewOfSection
@ stub MmMarkPhysicalMemoryAsBad
@ stub MmMarkPhysicalMemoryAsGood
@ stdcall MmPageEntireDriver(ptr)
@ stub MmPrefetchPages
@ stdcall MmProbeAndLockPages(ptr long long)
@ stub MmProbeAndLockProcessPages
@ stub MmProbeAndLockSelectedPages
@ stub MmProtectMdlSystemAddress
@ stdcall MmQuerySystemSize()
@ stub MmRemovePhysicalMemory
@ stdcall MmResetDriverPaging(ptr)
@ stub MmSectionObjectType
@ stub MmSecureVirtualMemory
@ stub MmSetAddressRangeModified
@ stub MmSetBankedSection
@ stub MmSizeOfMdl
@ stub MmSystemRangeStart
@ stub MmTrimAllSystemPagableMemory
@ stdcall MmUnlockPagableImageSection(ptr)
@ stdcall MmUnlockPages(ptr)
@ stdcall MmUnmapIoSpace(ptr long)
@ stub MmUnmapLockedPages
@ stub MmUnmapReservedMapping
@ stub MmUnmapVideoDisplay
@ stub MmUnmapViewInSessionSpace
@ stub MmUnmapViewInSystemSpace
@ stub MmUnmapViewOfSection
@ stub MmUnsecureVirtualMemory
@ stub MmUserProbeAddress
@ extern NlsAnsiCodePage ntdll.NlsAnsiCodePage
@ stub NlsLeadByteInfo
@ extern NlsMbCodePageTag ntdll.NlsMbCodePageTag
@ extern NlsMbOemCodePageTag ntdll.NlsMbOemCodePageTag
@ stub NlsOemCodePage
@ stub NlsOemLeadByteInfo
@ stdcall NtAddAtom(ptr long ptr)
@ stdcall NtAdjustPrivilegesToken(long long ptr long ptr ptr)
@ stdcall NtAllocateLocallyUniqueId(ptr)
@ stdcall NtAllocateUuids(ptr ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr long ptr long long)
@ stub NtBuildNumber
@ stdcall NtClose(long)
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtCreateEvent(ptr long ptr long long)
@ stdcall NtCreateFile(ptr long ptr ptr ptr long long long long ptr long)
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtDeleteAtom(long)
@ stdcall NtDeleteFile(ptr)
@ stdcall NtDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long ptr long long ptr)
@ stdcall NtFindAtom(ptr long ptr)
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stdcall NtFsControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stub NtGlobalFlag
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stub NtMakePermanentObject
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long ptr)
@ stdcall NtOpenThreadTokenEx(long long long long ptr)
@ stdcall NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall NtQueryInformationAtom(long long ptr long ptr)
@ stdcall NtQueryInformationFile(long ptr ptr long long)
@ stdcall NtQueryInformationProcess(long long ptr long ptr)
@ stdcall NtQueryInformationThread(long long ptr long ptr)
@ stdcall NtQueryInformationToken(long long ptr long ptr)
@ stub NtQueryQuotaInformationFile
@ stdcall NtQuerySecurityObject(long long ptr long ptr)
@ stdcall NtQuerySystemInformation(long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr)
@ stdcall NtSetEaFile(long ptr ptr long)
@ stdcall NtSetEvent(long ptr)
@ stdcall NtSetInformationFile(long ptr ptr long long)
@ stdcall NtSetInformationProcess(long long ptr long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stub NtSetQuotaInformationFile
@ stdcall NtSetSecurityObject(long long ptr)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall NtShutdownSystem(long)
@ stub NtTraceEvent
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stub NtVdmControl
@ stdcall NtWaitForSingleObject(long long ptr)
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stub ObAssignSecurity
@ stub ObCheckCreateObjectAccess
@ stub ObCheckObjectAccess
@ stub ObCloseHandle
@ stub ObCreateObject
@ stub ObCreateObjectType
@ stdcall ObDereferenceObject(ptr)
@ stub ObDereferenceSecurityDescriptor
@ stub ObFindHandleForObject
@ stdcall ObGetFilterVersion()
@ stub ObGetObjectSecurity
@ stdcall ObGetObjectType(ptr)
@ stub ObInsertObject
@ stub ObLogSecurityDescriptor
@ stub ObMakeTemporaryObject
@ stub ObOpenObjectByName
@ stub ObOpenObjectByPointer
@ stdcall ObQueryNameString(ptr ptr long ptr)
@ stub ObQueryObjectAuditingByHandle
@ stdcall ObReferenceObjectByHandle(long long ptr long ptr ptr)
@ stdcall ObReferenceObjectByName(ptr long ptr long ptr long ptr ptr)
@ stdcall ObReferenceObjectByPointer(ptr long ptr long)
@ stub ObReferenceSecurityDescriptor
@ stdcall ObRegisterCallbacks(ptr ptr)
@ stub ObReleaseObjectSecurity
@ stub ObSetHandleAttributes
@ stub ObSetSecurityDescriptorInfo
@ stub ObSetSecurityObjectByPointer
@ stdcall ObUnRegisterCallbacks(ptr)
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stub PoCallDriver
@ stub PoCancelDeviceNotify
@ stub PoQueueShutdownWorkItem
@ stub PoRegisterDeviceForIdleDetection
@ stub PoRegisterDeviceNotify
@ stub PoRegisterSystemState
@ stub PoRequestPowerIrp
@ stub PoRequestShutdownEvent
@ stub PoSetHiberRange
@ stdcall PoSetPowerState(ptr long long)
@ stub PoSetSystemState
@ stub PoShutdownBugCheck
@ stub PoStartNextPowerIrp
@ stub PoUnregisterSystemState
@ stdcall ProbeForRead(ptr long long)
@ stdcall ProbeForWrite(ptr long long)
@ stdcall PsAcquireProcessExitSynchronization(ptr)
@ stub PsAssignImpersonationToken
@ stub PsChargePoolQuota
@ stub PsChargeProcessNonPagedPoolQuota
@ stub PsChargeProcessPagedPoolQuota
@ stub PsChargeProcessPoolQuota
@ stub PsCreateSystemProcess
@ stdcall PsCreateSystemThread(ptr long ptr long ptr ptr ptr)
@ stub PsDereferenceImpersonationToken
@ stub PsDereferencePrimaryToken
@ stub PsDisableImpersonation
@ stub PsEstablishWin32Callouts
@ stub PsGetContextThread
@ stdcall PsGetCurrentProcess() IoGetCurrentProcess
@ stdcall PsGetCurrentProcessId()
@ stub PsGetCurrentProcessSessionId
@ stdcall PsGetCurrentThread() KeGetCurrentThread
@ stdcall PsGetCurrentThreadId()
@ stub PsGetCurrentThreadPreviousMode
@ stub PsGetCurrentThreadStackBase
@ stub PsGetCurrentThreadStackLimit
@ stub PsGetJobLock
@ stub PsGetJobSessionId
@ stub PsGetJobUIRestrictionsClass
@ stub PsGetProcessCreateTimeQuadPart
@ stub PsGetProcessDebugPort
@ stub PsGetProcessExitProcessCalled
@ stub PsGetProcessExitStatus
@ stub PsGetProcessExitTime
@ stdcall PsGetProcessId(ptr)
@ stub PsGetProcessImageFileName
@ stub PsGetProcessInheritedFromUniqueProcessId
@ stub PsGetProcessJob
@ stub PsGetProcessPeb
@ stub PsGetProcessPriorityClass
@ stub PsGetProcessSectionBaseAddress
@ stub PsGetProcessSecurityPort
@ stub PsGetProcessSessionId
@ stub PsGetProcessWin32Process
@ stub PsGetProcessWin32WindowStation
@ stdcall -arch=x86_64 PsGetProcessWow64Process(ptr)
@ stub PsGetThreadFreezeCount
@ stub PsGetThreadHardErrorsAreDisabled
@ stub PsGetThreadId
@ stub PsGetThreadProcess
@ stub PsGetThreadProcessId
@ stub PsGetThreadSessionId
@ stub PsGetThreadTeb
@ stub PsGetThreadWin32Thread
@ stdcall PsGetVersion(ptr ptr ptr ptr)
@ stdcall PsImpersonateClient(ptr ptr long long long)
@ stub PsInitialSystemProcess
@ stub PsIsProcessBeingDebugged
@ stub PsIsSystemThread
@ stub PsIsThreadImpersonating
@ stub PsIsThreadTerminating
@ stub PsJobType
@ stdcall PsLookupProcessByProcessId(ptr ptr)
@ stub PsLookupProcessThreadByCid
@ stub PsLookupThreadByThreadId
@ stub PsProcessType
@ stub PsReferenceImpersonationToken
@ stub PsReferencePrimaryToken
@ stdcall PsReleaseProcessExitSynchronization(ptr)
@ stdcall PsRemoveCreateThreadNotifyRoutine(ptr)
@ stdcall PsRemoveLoadImageNotifyRoutine(ptr)
@ stub PsRestoreImpersonation
@ stub PsReturnPoolQuota
@ stub PsReturnProcessNonPagedPoolQuota
@ stub PsReturnProcessPagedPoolQuota
@ stub PsRevertThreadToSelf
@ stub PsRevertToSelf
@ stub PsSetContextThread
@ stdcall PsSetCreateProcessNotifyRoutine(ptr long)
@ stdcall PsSetCreateProcessNotifyRoutineEx(ptr long)
@ stdcall PsSetCreateThreadNotifyRoutine(ptr)
@ stub PsSetJobUIRestrictionsClass
@ stub PsSetLegoNotifyRoutine
@ stdcall PsSetLoadImageNotifyRoutine(ptr)
@ stub PsSetProcessPriorityByClass
@ stub PsSetProcessPriorityClass
@ stub PsSetProcessSecurityPort
@ stub PsSetProcessWin32Process
@ stub PsSetProcessWindowStation
@ stub PsSetThreadHardErrorsAreDisabled
@ stub PsSetThreadWin32Thread
@ stdcall PsTerminateSystemThread(long)
@ stub PsThreadType
@ stdcall READ_REGISTER_BUFFER_UCHAR(ptr ptr long)
@ stub READ_REGISTER_BUFFER_ULONG
@ stub READ_REGISTER_BUFFER_USHORT
@ stub READ_REGISTER_UCHAR
@ stub READ_REGISTER_ULONG
@ stub READ_REGISTER_USHORT
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
@ stub RtlAddRange
@ stdcall RtlAllocateHeap(long long long)
@ stdcall RtlAnsiCharToUnicodeChar(ptr)
@ stdcall RtlAnsiStringToUnicodeSize(ptr)
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
@ stdcall RtlAssert(ptr ptr long str)
@ stdcall -norelay RtlCaptureContext(ptr)
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr)
@ stdcall RtlCharToInteger(ptr long ptr)
@ stdcall RtlCheckRegistryKey(long ptr)
@ stdcall RtlClearAllBits(ptr)
@ stub RtlClearBit
@ stdcall RtlClearBits(ptr long long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString(ptr ptr long)
@ stdcall RtlCompressBuffer(long ptr long ptr long long ptr ptr)
@ stub RtlCompressChunks
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr long)
@ stub RtlCopyRangeList
@ stdcall RtlCopySid(long ptr ptr)
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stdcall RtlCreateAtomTable(long ptr)
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stdcall RtlCreateRegistryKey(long wstr)
@ stdcall RtlCreateSecurityDescriptor(ptr long)
@ stub RtlCreateSystemVolumeInformationFolder
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stub RtlCustomCPToUnicodeN
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stub RtlDecompressChunks
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
@ stub RtlDelete
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteElementGenericTableAvl
@ stub RtlDeleteNoSplay
@ stub RtlDeleteOwnersRanges
@ stub RtlDeleteRange
@ stdcall RtlDeleteRegistryValue(long ptr ptr)
@ stub RtlDescribeChunk
@ stdcall RtlDestroyAtomTable(ptr)
@ stdcall RtlDestroyHeap(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlEmptyAtomTable(ptr long)
@ stdcall -arch=win32 -ret64 RtlEnlargedIntegerMultiply(long long)
@ stdcall -arch=win32 RtlEnlargedUnsignedDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlEnlargedUnsignedMultiply(long long)
@ stub RtlEnumerateGenericTable
@ stub RtlEnumerateGenericTableAvl
@ stub RtlEnumerateGenericTableLikeADirectory
@ stdcall RtlEnumerateGenericTableWithoutSplaying(ptr ptr)
@ stub RtlEnumerateGenericTableWithoutSplayingAvl
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualSid(ptr ptr)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall -arch=win32 -ret64 RtlExtendedIntegerMultiply(int64 long)
@ stdcall -arch=win32 -ret64 RtlExtendedLargeIntegerDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedMagicDivide(int64 int64 long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall RtlFillMemoryUlong(ptr long long)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stub RtlFindFirstRunClear
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(int64)
@ stdcall RtlFindLongestRunClear(ptr ptr)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(int64)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
@ stub RtlFindRange
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
@ stub RtlFindUnicodePrefix
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFreeAnsiString(ptr)
@ stdcall RtlFreeHeap(long long ptr)
@ stdcall RtlFreeOemString(ptr)
@ stub RtlFreeRangeList
@ stdcall RtlFreeUnicodeString(ptr)
@ stdcall RtlGUIDFromString(ptr ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stub RtlGetCallersAddress
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetDefaultCodePage
@ stub RtlGetElementGenericTable
@ stub RtlGetElementGenericTableAvl
@ stub RtlGetFirstRange
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stub RtlGetNextRange
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetSetBootStatusData
@ stdcall RtlGetVersion(ptr)
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stub RtlInitCodePageTable
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitializeBitMap(ptr ptr long)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stub RtlInitializeRangeList
@ stdcall RtlInitializeSid(ptr ptr long)
@ stub RtlInitializeUnicodePrefix
@ stub RtlInsertElementGenericTable
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ stub RtlInsertElementGenericTableFull
@ stub RtlInsertElementGenericTableFullAvl
@ stub RtlInsertUnicodePrefix
@ stdcall RtlInt64ToUnicodeString(int64 long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stub RtlIntegerToUnicode
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stub RtlInvertRangeList
@ stdcall RtlIpv4AddressToStringA(ptr ptr)
@ stdcall RtlIpv4AddressToStringExA(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringExW(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringW(ptr ptr)
@ stub RtlIpv4StringToAddressA
@ stub RtlIpv4StringToAddressExA
@ stdcall RtlIpv4StringToAddressExW(wstr long ptr ptr)
@ stdcall RtlIpv4StringToAddressW(wstr long ptr ptr)
@ stub RtlIpv6AddressToStringA
@ stub RtlIpv6AddressToStringExA
@ stub RtlIpv6AddressToStringExW
@ stub RtlIpv6AddressToStringW
@ stub RtlIpv6StringToAddressA
@ stub RtlIpv6StringToAddressExA
@ stdcall RtlIpv6StringToAddressExW(wstr ptr ptr ptr)
@ stub RtlIpv6StringToAddressW
@ stub RtlIsGenericTableEmpty
@ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stub RtlIsRangeAvailable
@ stub RtlIsValidOemCharacter
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(int64 int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(int64 int64 ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(int64 int64)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stub RtlLockBootStatusData
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stub RtlLookupElementGenericTable
@ stub RtlLookupElementGenericTableAvl
@ stub RtlLookupElementGenericTableFull
@ stub RtlLookupElementGenericTableFullAvl
@ stdcall RtlMapGenericMask(ptr ptr)
@ stub RtlMapSecurityErrorToNtStatus
@ stub RtlMergeRangeLists
@ stdcall RtlMoveMemory(ptr ptr long)
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
@ stub RtlNextUnicodePrefix
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlNumberGenericTableElements(ptr)
@ stub RtlNumberGenericTableElementsAvl
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stub RtlOemStringToCountedUnicodeString
@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlPinAtomInAtomTable(ptr long)
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stdcall RtlQueryTimeZoneInformation(ptr)
@ stdcall -norelay RtlRaiseException(ptr)
@ stdcall RtlRandom(ptr)
@ stdcall RtlRandomEx(ptr)
@ stub RtlRealPredecessor
@ stub RtlRealSuccessor
@ stub RtlRemoveUnicodePrefix
@ stub RtlReserveChunk
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
@ stub RtlSelfRelativeToAbsoluteSD2
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlSetAllBits(ptr)
@ stub RtlSetBit
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetTimeZoneInformation(ptr)
@ stdcall RtlSizeHeap(long long ptr)
@ stub RtlSplay
@ stdcall RtlStringFromGUID(ptr ptr)
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stub RtlSubtreePredecessor
@ stub RtlSubtreeSuccessor
@ stub RtlTestBit
@ stdcall RtlTimeFieldsToTime(ptr ptr)
@ stdcall RtlTimeToElapsedTimeFields(ptr ptr)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields(ptr ptr)
@ stub RtlTraceDatabaseAdd
@ stub RtlTraceDatabaseCreate
@ stub RtlTraceDatabaseDestroy
@ stub RtlTraceDatabaseEnumerate
@ stub RtlTraceDatabaseFind
@ stub RtlTraceDatabaseLock
@ stub RtlTraceDatabaseUnlock
@ stub RtlTraceDatabaseValidate
@ stdcall RtlUnicodeStringToAnsiSize(ptr)
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long)
@ stub RtlUnicodeStringToCountedOemString
@ stdcall RtlUnicodeStringToInteger(ptr long ptr)
@ stdcall RtlUnicodeStringToOemSize(ptr)
@ stdcall RtlUnicodeStringToOemString(ptr ptr long)
@ stub RtlUnicodeToCustomCPN
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long)
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long)
@ stub RtlUnlockBootStatusData
@ stdcall -norelay RtlUnwind(ptr ptr ptr ptr)
@ stdcall -arch=x86_64 RtlUnwindEx(ptr ptr ptr ptr ptr ptr)
@ stdcall RtlUpcaseUnicodeChar(long)
@ stdcall RtlUpcaseUnicodeString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long)
@ stub RtlUpcaseUnicodeToCustomCPN
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlVerifyVersionInfo(ptr long int64)
@ stub RtlVolumeDeviceToDosName
@ stub RtlWalkFrameChain
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr)
@ stdcall RtlxOemStringToUnicodeSize(ptr)
@ stdcall RtlxUnicodeStringToAnsiSize(ptr)
@ stdcall RtlxUnicodeStringToOemSize(ptr)
@ stub SeAccessCheck
@ stub SeAppendPrivileges
@ stub SeAssignSecurity
@ stub SeAssignSecurityEx
@ stub SeAuditHardLinkCreation
@ stub SeAuditingFileEvents
@ stub SeAuditingFileEventsWithContext
@ stub SeAuditingFileOrGlobalEvents
@ stub SeAuditingHardLinkEvents
@ stub SeAuditingHardLinkEventsWithContext
@ stub SeCaptureSecurityDescriptor
@ stub SeCaptureSubjectContext
@ stub SeCloseObjectAuditAlarm
@ stub SeCreateAccessState
@ stub SeCreateClientSecurity
@ stub SeCreateClientSecurityFromSubjectContext
@ stub SeDeassignSecurity
@ stub SeDeleteAccessState
@ stub SeDeleteObjectAuditAlarm
@ stub SeExports
@ stub SeFilterToken
@ stub SeFreePrivileges
@ stub SeImpersonateClient
@ stub SeImpersonateClientEx
@ stub SeLockSubjectContext
@ stub SeMarkLogonSessionForTerminationNotification
@ stub SeOpenObjectAuditAlarm
@ stub SeOpenObjectForDeleteAuditAlarm
@ stub SePrivilegeCheck
@ stub SePrivilegeObjectAuditAlarm
@ stub SePublicDefaultDacl
@ stub SeQueryAuthenticationIdToken
@ stub SeQueryInformationToken
@ stub SeQuerySecurityDescriptorInfo
@ stub SeQuerySessionIdToken
@ stub SeRegisterLogonSessionTerminatedRoutine
@ stub SeReleaseSecurityDescriptor
@ stub SeReleaseSubjectContext
@ stub SeSetAccessStateGenericMapping
@ stub SeSetSecurityDescriptorInfo
@ stub SeSetSecurityDescriptorInfoEx
@ stdcall SeSinglePrivilegeCheck(int64 long)
@ stub SeSystemDefaultDacl
@ stub SeTokenImpersonationLevel
@ stub SeTokenIsAdmin
@ stub SeTokenIsRestricted
@ stub SeTokenIsWriteRestricted
@ stub SeTokenObjectType
@ stub SeTokenType
@ stub SeUnlockSubjectContext
@ stub SeUnregisterLogonSessionTerminatedRoutine
@ stub SeValidSecurityDescriptor
@ stdcall -ret64 VerSetConditionMask(int64 long long)
@ stub VfFailDeviceNode
@ stub VfFailDriver
@ stub VfFailSystemBIOS
@ stub VfIsVerificationEnabled
@ stub WRITE_REGISTER_BUFFER_UCHAR
@ stub WRITE_REGISTER_BUFFER_ULONG
@ stub WRITE_REGISTER_BUFFER_USHORT
@ stub WRITE_REGISTER_UCHAR
@ stub WRITE_REGISTER_ULONG
@ stub WRITE_REGISTER_USHORT
@ stub WmiFlushTrace
@ stub WmiQueryTrace
@ stub WmiQueryTraceInformation
@ stub WmiStartTrace
@ stub WmiStopTrace
@ stub WmiTraceMessage
@ stub WmiTraceMessageVa
@ stub WmiUpdateTrace
@ stub XIPDispatch
@ stdcall -private ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) NtAccessCheckAndAuditAlarm
@ stub ZwAddBootEntry
@ stdcall -private ZwAdjustPrivilegesToken(long long ptr long ptr ptr) NtAdjustPrivilegesToken
@ stdcall -private ZwAlertThread(long) NtAlertThread
@ stdcall -private ZwAllocateVirtualMemory(long ptr long ptr long long) NtAllocateVirtualMemory
@ stdcall -private ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stdcall -private ZwCancelIoFile(long ptr) NtCancelIoFile
@ stdcall -private ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall -private ZwClearEvent(long) NtClearEvent
@ stdcall ZwClose(long) NtClose
@ stub ZwCloseObjectAuditAlarm
@ stdcall -private ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stdcall -private ZwCreateDirectoryObject(ptr long ptr) NtCreateDirectoryObject
@ stdcall -private ZwCreateEvent(ptr long ptr long long) NtCreateEvent
@ stdcall -private ZwCreateFile(ptr long ptr ptr ptr long long long long ptr long) NtCreateFile
@ stdcall -private ZwCreateJobObject(ptr long ptr) NtCreateJobObject
@ stdcall -private ZwCreateKey(ptr long ptr long ptr long ptr) NtCreateKey
@ stdcall -private ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall -private ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stdcall -private ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwDeleteBootEntry
@ stdcall -private ZwDeleteFile(ptr) NtDeleteFile
@ stdcall -private ZwDeleteKey(long) NtDeleteKey
@ stdcall -private ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall -private ZwDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long) NtDeviceIoControlFile
@ stdcall -private ZwDisplayString(ptr) NtDisplayString
@ stdcall -private ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall -private ZwDuplicateToken(long long ptr long long ptr) NtDuplicateToken
@ stub ZwEnumerateBootEntries
@ stdcall -private ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
@ stdcall -private ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stdcall -private ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall -private ZwFlushKey(long) NtFlushKey
@ stdcall -private ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stdcall -private ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall -private ZwFsControlFile(long long ptr ptr ptr long ptr long ptr long) NtFsControlFile
@ stdcall -private ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall -private ZwIsProcessInJob(long long) NtIsProcessInJob
@ stdcall ZwLoadDriver(ptr)
@ stdcall -private ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall -private ZwMakeTemporaryObject(long) NtMakeTemporaryObject
@ stdcall -private ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
@ stdcall -private ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall -private ZwOpenDirectoryObject(ptr long ptr) NtOpenDirectoryObject
@ stdcall -private ZwOpenEvent(ptr long ptr) NtOpenEvent
@ stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall -private ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall -private ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall -private ZwOpenProcess(ptr long ptr ptr) NtOpenProcess
@ stdcall -private ZwOpenProcessToken(long long ptr) NtOpenProcessToken
@ stdcall -private ZwOpenProcessTokenEx(long long long ptr) NtOpenProcessTokenEx
@ stdcall -private ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall -private ZwOpenSymbolicLinkObject(ptr long ptr) NtOpenSymbolicLinkObject
@ stdcall -private ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall -private ZwOpenThreadToken(long long long ptr) NtOpenThreadToken
@ stdcall -private ZwOpenThreadTokenEx(long long long long ptr) NtOpenThreadTokenEx
@ stdcall -private ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stdcall -private ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall -private ZwPulseEvent(long ptr) NtPulseEvent
@ stub ZwQueryBootEntryOrder
@ stub ZwQueryBootOptions
@ stdcall -private ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall -private ZwQueryDefaultUILanguage(ptr) NtQueryDefaultUILanguage
@ stdcall -private ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) NtQueryDirectoryFile
@ stdcall -private ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stdcall -private ZwQueryEaFile(long ptr ptr long long ptr long ptr long) NtQueryEaFile
@ stdcall -private ZwQueryFullAttributesFile(ptr ptr) NtQueryFullAttributesFile
@ stdcall -private ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stdcall -private ZwQueryInformationJobObject(long long ptr long ptr) NtQueryInformationJobObject
@ stdcall -private ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall -private ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall -private ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall -private ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
@ stdcall -private ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall -private ZwQueryObject(long long ptr long ptr) NtQueryObject
@ stdcall -private ZwQuerySection(long long ptr long ptr) NtQuerySection
@ stdcall -private ZwQuerySecurityObject(long long ptr long ptr) NtQuerySecurityObject
@ stdcall -private ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stdcall -private ZwQuerySystemInformation(long ptr long ptr) NtQuerySystemInformation
@ stdcall -private ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall -private ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall -private ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall -private ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stdcall -private ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
@ stdcall -private ZwResetEvent(long ptr) NtResetEvent
@ stdcall -private ZwRestoreKey(long long long) NtRestoreKey
@ stdcall -private ZwSaveKey(long long) NtSaveKey
@ stub ZwSaveKeyEx
@ stub ZwSetBootEntryOrder
@ stub ZwSetBootOptions
@ stdcall -private ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stdcall -private ZwSetDefaultUILanguage(long) NtSetDefaultUILanguage
@ stdcall -private ZwSetEaFile(long ptr ptr long) NtSetEaFile
@ stdcall -private ZwSetEvent(long ptr) NtSetEvent
@ stdcall -private ZwSetInformationFile(long ptr ptr long long) NtSetInformationFile
@ stdcall -private ZwSetInformationJobObject(long long ptr long) NtSetInformationJobObject
@ stdcall -private ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall -private ZwSetInformationProcess(long long ptr long) NtSetInformationProcess
@ stdcall -private ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stdcall -private ZwSetSecurityObject(long long ptr) NtSetSecurityObject
@ stdcall -private ZwSetSystemInformation(long ptr long) NtSetSystemInformation
@ stdcall -private ZwSetSystemTime(ptr ptr) NtSetSystemTime
@ stdcall -private ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall -private ZwSetValueKey(long ptr long long ptr long) NtSetValueKey
@ stdcall -private ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stdcall -private ZwTerminateJobObject(long long) NtTerminateJobObject
@ stdcall -private ZwTerminateProcess(long long) NtTerminateProcess
@ stub ZwTranslateFilePath
@ stdcall ZwUnloadDriver(ptr)
@ stdcall -private ZwUnloadKey(ptr) NtUnloadKey
@ stdcall -private ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stdcall -private ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
@ stdcall ZwWaitForSingleObject(long long ptr) NtWaitForSingleObject
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stdcall -private ZwYieldExecution() NtYieldExecution
@ stdcall -private -arch=arm,x86_64 -norelay __chkstk()
@ cdecl -private -arch=i386 _CIcos() msvcrt._CIcos
@ cdecl -private -arch=i386 _CIsin() msvcrt._CIsin
@ cdecl -private -arch=i386 _CIsqrt() msvcrt._CIsqrt
@ cdecl -private _abnormal_termination() msvcrt._abnormal_termination
@ stdcall -private -arch=i386 -ret64 _alldiv(int64 int64)
@ stdcall -private -arch=i386 -norelay _alldvrm(int64 int64)
@ stdcall -private -arch=i386 -ret64 _allmul(int64 int64)
@ stdcall -private -arch=i386 -norelay _alloca_probe()
@ stdcall -private -arch=i386 -ret64 _allrem(int64 int64)
@ stdcall -private -arch=i386 -ret64 _allshl(int64 long)
@ stdcall -private -arch=i386 -ret64 _allshr(int64 long)
@ stdcall -private -arch=i386 -ret64 _aulldiv(int64 int64)
@ stdcall -private -arch=i386 -norelay _aulldvrm(int64 int64)
@ stdcall -private -arch=i386 -ret64 _aullrem(int64 int64)
@ stdcall -private -arch=i386 -ret64 _aullshr(int64 long)
@ stdcall -private -arch=i386 -norelay _chkstk()
@ cdecl -private -arch=i386 _except_handler2(ptr ptr ptr ptr) msvcrt._except_handler2
@ cdecl -private -arch=i386 _except_handler3(ptr ptr ptr ptr) msvcrt._except_handler3
@ cdecl -private -arch=i386 _global_unwind2(ptr) msvcrt._global_unwind2
@ cdecl -private _itoa(long ptr long) msvcrt._itoa
@ cdecl -private _itow(long ptr long) msvcrt._itow
@ cdecl -private -arch=x86_64 _local_unwind(ptr ptr) msvcrt._local_unwind
@ cdecl -private -arch=i386 _local_unwind2(ptr long) msvcrt._local_unwind2
@ cdecl -private _purecall() msvcrt._purecall
@ varargs -private _snprintf(ptr long str) msvcrt._snprintf
@ varargs -private _snwprintf(ptr long wstr) msvcrt._snwprintf
@ cdecl -private _stricmp(str str) NTOSKRNL__stricmp
@ cdecl -private _strlwr(str) msvcrt._strlwr
@ cdecl -private _strnicmp(str str long) NTOSKRNL__strnicmp
@ cdecl -private _strnset(str long long) msvcrt._strnset
@ cdecl -private _strrev(str) msvcrt._strrev
@ cdecl -private _strset(str long) msvcrt._strset
@ cdecl -private _strupr(str) msvcrt._strupr
@ cdecl _vsnprintf(ptr long str ptr) msvcrt._vsnprintf
@ cdecl -private _vsnwprintf(ptr long wstr ptr) msvcrt._vsnwprintf
@ cdecl -private _wcsicmp(wstr wstr) msvcrt._wcsicmp
@ cdecl -private _wcslwr(wstr) msvcrt._wcslwr
@ cdecl -private _wcsnicmp(wstr wstr long) NTOSKRNL__wcsnicmp
@ cdecl -private _wcsnset(wstr long long) msvcrt._wcsnset
@ cdecl -private _wcsrev(wstr) msvcrt._wcsrev
@ cdecl -private _wcsupr(wstr) msvcrt._wcsupr
@ cdecl -private atoi(str) msvcrt.atoi
@ cdecl -private atol(str) msvcrt.atol
@ cdecl -private isdigit(long) msvcrt.isdigit
@ cdecl -private islower(long) msvcrt.islower
@ cdecl -private isprint(long) msvcrt.isprint
@ cdecl -private isspace(long) msvcrt.isspace
@ cdecl -private isupper(long) msvcrt.isupper
@ cdecl -private isxdigit(long) msvcrt.isxdigit
@ cdecl -private mbstowcs(ptr str long) msvcrt.mbstowcs
@ cdecl -private mbtowc(ptr str long) msvcrt.mbtowc
@ cdecl -private memchr(ptr long long) msvcrt.memchr
@ cdecl -private memcpy(ptr ptr long) NTOSKRNL_memcpy
@ cdecl -private memmove(ptr ptr long) msvcrt.memmove
@ cdecl -private memset(ptr long long) NTOSKRNL_memset
@ cdecl -private qsort(ptr long long ptr) msvcrt.qsort
@ cdecl -private rand() msvcrt.rand
@ varargs -private sprintf(ptr str) msvcrt.sprintf
@ cdecl -private srand(long) msvcrt.srand
@ cdecl -private strcat(str str) msvcrt.strcat
@ cdecl -private strchr(str long) msvcrt.strchr
@ cdecl -private strcmp(str str) msvcrt.strcmp
@ cdecl -private strcpy(ptr str) msvcrt.strcpy
@ cdecl -private strlen(str) msvcrt.strlen
@ cdecl -private strncat(str str long) msvcrt.strncat
@ cdecl -private strncmp(str str long) msvcrt.strncmp
@ cdecl -private strncpy(ptr str long) msvcrt.strncpy
@ cdecl -private strrchr(str long) msvcrt.strrchr
@ cdecl -private strspn(str str) msvcrt.strspn
@ cdecl -private strstr(str str) msvcrt.strstr
@ varargs -private swprintf(ptr wstr) msvcrt.swprintf
@ cdecl -private tolower(long) msvcrt.tolower
@ cdecl -private toupper(long) msvcrt.toupper
@ cdecl -private towlower(long) msvcrt.towlower
@ cdecl -private towupper(long) msvcrt.towupper
@ stdcall vDbgPrintEx(long long str ptr)
@ stdcall vDbgPrintExWithPrefix(str long long str ptr)
@ cdecl -private vsprintf(ptr str ptr) msvcrt.vsprintf
@ cdecl -private wcscat(wstr wstr) msvcrt.wcscat
@ cdecl -private wcschr(wstr long) msvcrt.wcschr
@ cdecl -private wcscmp(wstr wstr) msvcrt.wcscmp
@ cdecl -private wcscpy(ptr wstr) msvcrt.wcscpy
@ cdecl -private wcscspn(wstr wstr) msvcrt.wcscspn
@ cdecl -private wcslen(wstr) msvcrt.wcslen
@ cdecl -private wcsncat(wstr wstr long) msvcrt.wcsncat
@ cdecl -private wcsncmp(wstr wstr long) NTOSKRNL_wcsncmp
@ cdecl -private wcsncpy(ptr wstr long) msvcrt.wcsncpy
@ cdecl -private wcsrchr(wstr long) msvcrt.wcsrchr
@ cdecl -private wcsspn(wstr wstr) msvcrt.wcsspn
@ cdecl -private wcsstr(wstr wstr) msvcrt.wcsstr
@ cdecl -private wcstombs(ptr ptr long) msvcrt.wcstombs
@ cdecl -private wctomb(ptr long) msvcrt.wctomb

################################################################
# Wine internal extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

@ cdecl wine_ntoskrnl_main_loop(long)
