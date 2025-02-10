#ifndef CONVERSION_STATUS_H
#define CONVERSION_STATUS_H

#include <string>
#include <memory>

class ConvertStatus {
protected:
    bool successful;

public:
    ConvertStatus(bool _successful): successful(_successful) {}
    virtual ~ConvertStatus() = default;

    bool isSuccessful() const { return successful; }
    virtual const char *errorMessage() const = 0;
};

class SuccessStatus: public ConvertStatus {
public:
    SuccessStatus(): ConvertStatus(true) {}
    const char *errorMessage() const override { return ""; }
};

class UnsupportedGridTypeStatus: public ConvertStatus {
public:
    UnsupportedGridTypeStatus(): ConvertStatus(false) {}
    const char *errorMessage() const override { return "Unsupported grid type"; }
};

class UnsupportedCellTypeStatus: public ConvertStatus {
public:
    UnsupportedCellTypeStatus(): ConvertStatus(false) {}
    const char *errorMessage() const override { return "Unsupported cell type"; }
};

class UnsupportedFieldTypeStatus: public ConvertStatus {
public:
    UnsupportedFieldTypeStatus(): ConvertStatus(false) {}
    const char *errorMessage() const override { return "Unsupported field type"; }
};

class OtherErrorStatus: public ConvertStatus {
public:
    OtherErrorStatus(): ConvertStatus(false) {}
    const char *errorMessage() const override { return "Other error"; }
};

// Factory methods to create instances of the status objects
inline std::unique_ptr<ConvertStatus> Success()
{
    return std::make_unique<SuccessStatus>();
}

inline std::unique_ptr<ConvertStatus> UnsupportedGridType()
{
    return std::make_unique<UnsupportedGridTypeStatus>();
}

inline std::unique_ptr<ConvertStatus> UnsupportedCellType()
{
    return std::make_unique<UnsupportedCellTypeStatus>();
}

inline std::unique_ptr<ConvertStatus> UnsupportedFieldType()
{
    return std::make_unique<UnsupportedFieldTypeStatus>();
}

inline std::unique_ptr<ConvertStatus> OtherError()
{
    return std::make_unique<OtherErrorStatus>();
}


#endif // CONVERSION_STATUS_H
