#pragma once

#include <string>

#include "Artifice/Core/Core.h"

enum class VariantType
{
    None,
    Bool,
    Int32,
    Int64,
    Uint32,
    Uint64,
    Float,
    Double,
    String
};

class Variant
{

private:
    VariantType m_Type = VariantType::None;

    union ValueUnion
    {
        bool Bool;
        int32 Int32;
        int64 Int64;
        uint32 Uint32;
        uint64 Uint64;
        float Float;
        double Double;
        const char* String;
    } m_ValueData;

public:
    Variant() = default;
    Variant(std::string value)
    {
        m_Type = VariantType::String;
        m_ValueData.String = value.c_str();
    }
    Variant(const char* value)
    {
        m_Type = VariantType::String;
        m_ValueData.String = value;
    }
    Variant(bool value)
    {
        m_Type = VariantType::Bool;
        m_ValueData.Bool = value;
    }
    Variant(int32 value)
    {
        m_Type = VariantType::Int32;
        m_ValueData.Int32 = value;
    }
    Variant(int64 value)
    {
        m_Type = VariantType::Int64;
        m_ValueData.Int64 = value;
    }
    Variant(uint32 value)
    {
        m_Type = VariantType::Uint32;
        m_ValueData.Uint32 = value;
    }
    Variant(uint64 value)
    {
        m_Type = VariantType::Uint64;
        m_ValueData.Uint64 = value;
    }
    Variant(float value)
    {
        m_Type = VariantType::Float;
        m_ValueData.Float = value;
    }
    Variant(double value)
    {
        m_Type = VariantType::Double;
        m_ValueData.Double = value;
    }

    VariantType GetType() const { return m_Type; }

    Variant& operator=(const Variant& other)
    {
        m_Type = other.m_Type;
        m_ValueData = other.m_ValueData;

        return *this;
    }

    bool GetBool() const 
    { 
        return m_ValueData.Bool; 
    }
    int32 GetInt32() const 
    { 
        return m_ValueData.Int32; 
    }
    int64 GetInt64() const 
    { 
        return m_ValueData.Int64; 
    }
    uint32 GetUint32() const 
    { 
        return m_ValueData.Uint32; 
    }
    uint64 GetUint64() const 
    { 
        return m_ValueData.Uint64; 
    }
    float GetFloat() const 
    { 
        return m_ValueData.Float; 
    }
    double GetDouble() const 
    { 
        return m_ValueData.Double; 
    }
    const char* GetString() const 
    { 
        return m_ValueData.String; 
    }
};