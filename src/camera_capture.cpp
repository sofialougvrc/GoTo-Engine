#include "gte/camera_capture.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace gte {

FakeCamera::FakeCamera(std::string fixture_image_path)
    : fixture_image_path_(std::move(fixture_image_path)) {}

std::string FakeCamera::captureImage(const std::string& output_path) {
    if (!std::filesystem::exists(fixture_image_path_)) {
        throw std::runtime_error("FakeCamera fixture does not exist: " + fixture_image_path_);
    }

    std::filesystem::copy_file(
        fixture_image_path_,
        output_path,
        std::filesystem::copy_options::overwrite_existing);
    return output_path;
}

const std::string& FakeCamera::fixtureImagePath() const {
    return fixture_image_path_;
}

std::string ZwoCamera::captureImage(const std::string& output_path) {
    (void)output_path;
    throw std::runtime_error("ZwoCamera requires the ZWO ASI SDK and camera hardware");
}

} // namespace gte
