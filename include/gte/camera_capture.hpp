#pragma once

#include <string>

namespace gte {

class CameraCapture {
public:
    virtual std::string captureImage(const std::string& output_path) = 0;
    virtual ~CameraCapture() = default;
};

class FakeCamera : public CameraCapture {
public:
    explicit FakeCamera(std::string fixture_image_path);

    std::string captureImage(const std::string& output_path) override;
    const std::string& fixtureImagePath() const;

private:
    std::string fixture_image_path_;
};

class ZwoCamera : public CameraCapture {
public:
    std::string captureImage(const std::string& output_path) override;
};

} // namespace gte
