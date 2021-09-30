#ifndef MONODEBUGGERPRIVATE
#define MONODEBUGGERPRIVATE

/*
* Copyright 2021 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the MonoTools library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

enum SuspendPolicy {
    SUSPEND_POLICY_NONE = 0,
    SUSPEND_POLICY_EVENT_THREAD = 1,
    SUSPEND_POLICY_ALL = 2
};

enum ModifierKind {
    MOD_KIND_COUNT = 1,
    MOD_KIND_THREAD_ONLY = 3,
    MOD_KIND_LOCATION_ONLY = 7,
    MOD_KIND_EXCEPTION_ONLY = 8,
    MOD_KIND_STEP = 10,
    MOD_KIND_ASSEMBLY_ONLY = 11,
    MOD_KIND_SOURCE_FILE_ONLY = 12,
    MOD_KIND_TYPE_NAME_ONLY = 13,
    MOD_KIND_NONE = 14
};

enum StepDepth {
    STEP_DEPTH_INTO = 0,
    STEP_DEPTH_OVER = 1,
    STEP_DEPTH_OUT = 2
};

enum StepSize {
    STEP_SIZE_MIN = 0,
    STEP_SIZE_LINE = 1
};

enum DebuggerTokenType {
    TOKEN_TYPE_STRING = 0,
    TOKEN_TYPE_TYPE = 1,
    TOKEN_TYPE_FIELD = 2,
    TOKEN_TYPE_METHOD = 3,
    TOKEN_TYPE_UNKNOWN = 4
} ;

enum ValueTypeId {
    VALUE_TYPE_ID_NULL = 0xf0,
    VALUE_TYPE_ID_TYPE = 0xf1,
    VALUE_TYPE_ID_PARENT_VTYPE = 0xf2
} ;

enum StackFrameFlags {
    FRAME_FLAG_DEBUGGER_INVOKE = 1,

    // Use to allow the debugger to display managed-to-native transitions in stack frames.
    FRAME_FLAG_NATIVE_TRANSITION = 2
};

enum ValueTypeCodes {
    VT_End		 = 0x00,
    VT_Void		= 0x01,
    VT_Boolean	 = 0x02,
    VT_Char		= 0x03,
    VT_I1		  = 0x04,
    VT_U1		  = 0x05,
    VT_I2		  = 0x06,
    VT_U2		  = 0x07,
    VT_I4		  = 0x08,
    VT_U4		  = 0x09,
    VT_I8		  = 0x0a,
    VT_U8		  = 0x0b,
    VT_R4		  = 0x0c,
    VT_R8		  = 0x0d,
    VT_String	  = 0x0e,
    VT_Ptr		 = 0x0f,
    VT_ByRef	   = 0x10,
    VT_ValueType   = 0x11,
    VT_Class	   = 0x12,
    VT_Var        = 0x13,
    VT_Array	   = 0x14,
    VT_GenericInst = 0x15,
    VT_TypedByRef  = 0x16,
    VT_I		   = 0x18,
    VT_U		   = 0x19,
    VT_FnPtr	   = 0x1b,
    VT_Object	  = 0x1c,
    VT_SzArray	 = 0x1d,
    VT_MVar       = 0x1e,
    VT_CModReqD	= 0x1f,
    VT_CModOpt	 = 0x20,
    VT_Internal	= 0x21,
    VT_Modifier	= 0x40,
    VT_Sentinel	= 0x41,
    VT_Pinned	  = 0x45,

    VT_Type		= 0x50,
    VT_Boxed	   = 0x51,
    VT_Enum		= 0x55
};

enum InvokeFlags {
    INVOKE_FLAG_DISABLE_BREAKPOINTS = 1,
    INVOKE_FLAG_SINGLE_THREADED = 2,

    // Allow for returning the changed value types after an invocation
    INVOKE_FLAG_RETURN_OUT_THIS = 4,

    // Allows the return of modified value types after invocation
    INVOKE_FLAG_RETURN_OUT_ARGS = 8,

    // Performs a virtual method invocation
    INVOKE_FLAG_VIRTUAL = 16
};

enum MonoThreadState {
    ThreadState_Running = 0x00000000,
    ThreadState_StopRequested = 0x00000001,
    ThreadState_SuspendRequested = 0x00000002,
    ThreadState_Background = 0x00000004,
    ThreadState_Unstarted = 0x00000008,
    ThreadState_Stopped = 0x00000010,
    ThreadState_WaitSleepJoin = 0x00000020,
    ThreadState_Suspended = 0x00000040,
    ThreadState_AbortRequested = 0x00000080,
    ThreadState_Aborted = 0x00000100
};

enum CommandSet {
    CMD_SET_VM = 1,
    CMD_SET_OBJECT_REF = 9,
    CMD_SET_STRING_REF = 10,
    CMD_SET_THREAD = 11,
    CMD_SET_ARRAY_REF = 13,
    CMD_SET_EVENT_REQUEST = 15,
    CMD_SET_STACK_FRAME = 16,
    CMD_SET_APPDOMAIN = 20,
    CMD_SET_ASSEMBLY = 21,
    CMD_SET_METHOD = 22,
    CMD_SET_TYPE = 23,
    CMD_SET_MODULE = 24,
    CMD_SET_FIELD = 25,
    CMD_SET_EVENT = 64
};

enum CmdVM {
    CMD_VM_VERSION = 1,
    CMD_VM_ALL_THREADS = 2,
    CMD_VM_SUSPEND = 3,
    CMD_VM_RESUME = 4,
    CMD_VM_EXIT = 5,
    CMD_VM_DISPOSE = 6,
    CMD_VM_INVOKE_METHOD = 7,
    CMD_VM_SET_PROTOCOL_VERSION = 8,
    CMD_VM_ABORT_INVOKE = 9,
    CMD_VM_SET_KEEPALIVE = 10,
    CMD_VM_GET_TYPES_FOR_SOURCE_FILE = 11,
    CMD_VM_GET_TYPES = 12,
    CMD_VM_INVOKE_METHODS = 13,
    CMD_VM_START_BUFFERING = 14,
    CMD_VM_STOP_BUFFERING = 15
};

enum CmdThread {
    CMD_THREAD_GET_FRAME_INFO = 1,
    CMD_THREAD_GET_NAME = 2,
    CMD_THREAD_GET_STATE = 3,
    CMD_THREAD_GET_INFO = 4,
    CMD_THREAD_GET_ID = 5,
    CMD_THREAD_GET_TID = 6,
    CMD_THREAD_SET_IP = 7
};

enum CmdEvent {
    CMD_EVENT_REQUEST_SET = 1,
    CMD_EVENT_REQUEST_CLEAR = 2,
    CMD_EVENT_REQUEST_CLEAR_ALL_BREAKPOINTS = 3
};

enum CmdComposite {
    CMD_COMPOSITE = 100
};

enum CmdAppDomain {
    CMD_APPDOMAIN_GET_ROOT_DOMAIN = 1,
    CMD_APPDOMAIN_GET_FRIENDLY_NAME = 2,
    CMD_APPDOMAIN_GET_ASSEMBLIES = 3,
    CMD_APPDOMAIN_GET_ENTRY_ASSEMBLY = 4,
    CMD_APPDOMAIN_CREATE_STRING = 5,
    CMD_APPDOMAIN_GET_CORLIB = 6,
    CMD_APPDOMAIN_CREATE_BOXED_VALUE = 7
};

enum CmdAssembly {
    CMD_ASSEMBLY_GET_LOCATION = 1,
    CMD_ASSEMBLY_GET_ENTRY_POINT = 2,
    CMD_ASSEMBLY_GET_MANIFEST_MODULE = 3,
    CMD_ASSEMBLY_GET_OBJECT = 4,
    CMD_ASSEMBLY_GET_TYPE = 5,
    CMD_ASSEMBLY_GET_NAME = 6
};

enum CmdModule {
    CMD_MODULE_GET_INFO = 1,
};

enum CmdField {
    CMD_FIELD_GET_INFO = 1,
};

enum CmdMethod {
    CMD_METHOD_GET_NAME = 1,
    CMD_METHOD_GET_DECLARING_TYPE = 2,
    CMD_METHOD_GET_DEBUG_INFO = 3,
    CMD_METHOD_GET_PARAM_INFO = 4,
    CMD_METHOD_GET_LOCALS_INFO = 5,
    CMD_METHOD_GET_INFO = 6,
    CMD_METHOD_GET_BODY = 7,
    CMD_METHOD_RESOLVE_TOKEN = 8,
    CMD_METHOD_GET_CATTRS = 9,
    CMD_METHOD_MAKE_GENERIC_METHOD = 10
};

enum CmdType {
    CMD_TYPE_GET_INFO = 1,
    CMD_TYPE_GET_METHODS = 2,
    CMD_TYPE_GET_FIELDS = 3,
    CMD_TYPE_GET_VALUES = 4,
    CMD_TYPE_GET_OBJECT = 5,
    CMD_TYPE_GET_SOURCE_FILES = 6,
    CMD_TYPE_SET_VALUES = 7,
    CMD_TYPE_IS_ASSIGNABLE_FROM = 8,
    CMD_TYPE_GET_PROPERTIES = 9,
    CMD_TYPE_GET_CATTRS = 10,
    CMD_TYPE_GET_FIELD_CATTRS = 11,
    CMD_TYPE_GET_PROPERTY_CATTRS = 12,
    CMD_TYPE_GET_SOURCE_FILES_2 = 13,
    CMD_TYPE_GET_VALUES_2 = 14,
    CMD_TYPE_GET_METHODS_BY_NAME_FLAGS = 15,
    CMD_TYPE_GET_INTERFACES = 16,
    CMD_TYPE_GET_INTERFACE_MAP = 17,
    CMD_TYPE_IS_INITIALIZED = 18,
    CMD_TYPE_CREATE_INSTANCE = 19
};

enum CmdStackFrame {
    CMD_STACK_FRAME_GET_VALUES = 1,
    CMD_STACK_FRAME_GET_THIS = 2,
    CMD_STACK_FRAME_SET_VALUES = 3,
    CMD_STACK_FRAME_GET_DOMAIN = 4,
};

enum CmdArray {
    CMD_ARRAY_REF_GET_LENGTH = 1,
    CMD_ARRAY_REF_GET_VALUES = 2,
    CMD_ARRAY_REF_SET_VALUES = 3,
};

enum CmdString {
    CMD_STRING_REF_GET_VALUE = 1,
    CMD_STRING_REF_GET_LENGTH = 2,
    CMD_STRING_REF_GET_CHARS = 3
};

enum CmdObject {
    CMD_OBJECT_REF_GET_TYPE = 1,
    CMD_OBJECT_REF_GET_VALUES = 2,
    CMD_OBJECT_REF_IS_COLLECTED = 3,
    CMD_OBJECT_REF_GET_ADDRESS = 4,
    CMD_OBJECT_REF_GET_DOMAIN = 5,
    CMD_OBJECT_REF_SET_VALUES = 6,
    CMD_OBJECT_REF_GET_INFO = 7,
};

enum ErrorCode {
    ERR_NONE = 0,
    ERR_INVALID_OBJECT = 20,
    ERR_INVALID_FIELDID = 25,
    ERR_INVALID_FRAMEID = 30,
    ERR_NOT_IMPLEMENTED = 100,
    ERR_NOT_SUSPENDED = 101,
    ERR_INVALID_ARGUMENT = 102,
    ERR_UNLOADED = 103,
    ERR_NO_INVOCATION = 104,
    ERR_ABSENT_INFORMATION = 105,
    ERR_NO_SEQ_POINT_AT_IL_OFFSET = 106,
    ERR_LOADER_ERROR = 200, /*XXX extend the protocol to pass this information down the pipe */
};

#define MAJOR_VERSION 2
#define MINOR_VERSION 38

#define METHOD_IMPL_ATTRIBUTE_IL                   0x0000
#define METHOD_IMPL_ATTRIBUTE_NATIVE               0x0001
#define METHOD_IMPL_ATTRIBUTE_OPTIL                0x0002
#define METHOD_IMPL_ATTRIBUTE_RUNTIME              0x0003

#define METHOD_ATTRIBUTE_STATIC                    0x0010
#define METHOD_ATTRIBUTE_VIRTUAL                   0x0040

#define FIELD_ATTRIBUTE_STATIC                0x0010

#endif // MONODEBUGGERPRIVATE

