/*******************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/
#ifndef __LOADER_ERROR_CODE_H
#define __LOADER_ERROR_CODE_H


#define XMON_LOADER_SUCCESS 0

/* error code for starter */
#define  STARTER_DEADLOOP                           0xDEADDEAD
#define  STARTER_INVALID_MB_MAGIC                   0xDEAD0000
#define  STARTER_MODULE_MEM_OVERLAP_WITH_LOADER     0xDEAD0001
#define  STARTER_MB_FLAG_CONFLICT_ERROR             0xDEAD0002
#define  STARTER_NO_MEMORY_MAP                      0xDEAD0003
#define  STARTER_NO_VMX_SUPPORT                     0xDEAD0004
#define  STARTER_NO_AVAIL_LOADER_MEM_IN_E820        0xDEAD0005
#define  STARTER_NO_AVAIL_RUNTIME_MEM_IN_E820       0xDEAD0006
#define  STARTER_OVERLAP_LOADER_RUNTIME_MEM         0xDEAD0007
#define  STARTER_FAILED_TO_RELOCATE_XMON_LOADER     0xDEAD0008
#define  STARTER_FAILED_TO_GET_XMON_LOADER_IMG_INFO 0xDEAD0009
#define  STARTER_XMON_LOADER_IMG_SIZE_TOO_SMALL     0xDEAD000A
#define  STARTER_XMON_NO_FILE_MAPPING_HEADER        0xDEAD000B
#define  STARTER_SOME_XMON_BINARY_MISSING           0xDEAD000C
#define  STARTER_INVALID_XMON_LOADER_BIN_ADDR       0xDEAD000D
#define  STARTER_MODULE_XMON_LOADER_FILE_MISSING    0xDEAD000E
#define  STARTER_STARTAP_IMG_SIZE_TOO_SMALL         0xDEAD000F
#define  STARTER_MODULE_STARTAP_FILE_MISSING        0xDEAD0010
#define  STARTER_MODULE_XMON_Z_FILE_MISSING         0xDEAD0011
#define  STARTER_STRUCTURE_SIZE_MISMATCH            0xDEAD0012


/* error code for xmon_loader */
#define XMON_LOADER_FAILED_TO_GET_XMON_IMG_INFO          0xC000DEAD
#define XMON_LOADER_FAILED_TO_DECOMPRESS_XMON            0xC001DEAD
#define XMON_LOADER_FAILED_TO_LOAD_XMON_IMG              0xC002DEAD
#define XMON_LOADER_FAILED_TO_GET_STARTAP_IMG_INFO       0xC003DEAD
#define XMON_LOADER_FAILED_TO_LOAD_STARTAP_IMG           0xC004DEAD
#define XMON_LOADER_FAILED_TO_SETUP_STARTUP_ENV          0xC005DEAD
#define XMON_LOADER_HEAP_OUT_OF_MEMORY                   0xC006DEAD
#define XMON_LOADER_FAILED_TO_CALL_THUNK                 0xC007DEAD
#define XMON_LOADER_NO_VALID_AP_WAKEUP_CODE_ADDRESS      0xC008DEAD
#define XMON_LOADER_FAILED_TO_INIT_PROTOCOL_OPS          0xC00DDEAD
#define XMON_LOADER_FAILED_TO_INIT_INIT32                0xC00EDEAD

#define XMON_FAILED_TO_SETUP_PRIMARY_GUEST_ENV     0xC009DEAD
#define XMON_FAILED_TO_SETUP_SECONDARY_GUESTS_ENV  0xC00ADEAD
#define XMON_FAILED_TO_HIDE_RUNTIME_MEMORY         0xC00BDEAD
#define XMON_CODING_ERROR                          0xC00CDEAD


#define XMON_STRING_INVALID_PARAMETER              0xB000DEAD
#define XMON_CMDLINE_VALUE_STRING_LEN_TOO_LARGE    0xB001DEAD


#endif
