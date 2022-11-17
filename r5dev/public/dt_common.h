#pragma once

enum class SendPropType : int {
    DPT_Int = 0,
    DPT_Float,
    DPT_Vector,
    DPT_VectorXY,
    DPT_String,
    DPT_Array,
    DPT_Quaternion,
    DPT_Int64,
    DPT_Ticks,
    DPT_Time,
    DPT_DataTable,
    DPT_NUMSendPropTypes
};