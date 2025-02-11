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
    const char *errorMessage() const override { return "Success"; }
};

class ErrorStatus: public ConvertStatus {
private:
    const char *message;

public:
    ErrorStatus(const char *_message): ConvertStatus(false), message(_message) {}
    const char *errorMessage() const override { return message; }
};

// Factory methods to create instances of the status objects
inline std::unique_ptr<ConvertStatus> Success()
{
    return std::make_unique<SuccessStatus>();
}

inline std::unique_ptr<ConvertStatus> Error(const char *message)
{
    return std::make_unique<ErrorStatus>(message);
}

#endif // CONVERSION_STATUS_H
