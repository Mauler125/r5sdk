//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef CPU_H
#define CPU_H

bool CheckSSE3Technology(void);
bool CheckSSSE3Technology(void);
bool CheckSSE41Technology(void);
bool CheckSSE42Technology(void);
bool CheckSSE4aTechnology(void);

const char* GetProcessorVendorId(void);
const char* GetProcessorBrand(void);

const CPUInformation& GetCPUInformation(void);

#endif // CPU_H
