#include "gte/camera_capture.hpp"
#include "gte/plate_solver.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool expectTrue(const std::string& name, bool condition) {
    if (condition) {
        return true;
    }

    std::cerr << name << " failed\n";
    return false;
}

bool expectNear(const std::string& name, double actual, double expected, double tolerance) {
    const double error = std::abs(actual - expected);
    if (error <= tolerance) {
        return true;
    }

    std::cerr << name << " failed: expected " << expected
              << ", got " << actual << ", error " << error << '\n';
    return false;
}

template <typename Callable>
bool expectThrowsRuntimeError(const std::string& name, Callable callable) {
    try {
        callable();
    } catch (const std::runtime_error&) {
        return true;
    } catch (...) {
        std::cerr << name << " failed: threw unexpected exception type\n";
        return false;
    }

    std::cerr << name << " failed: did not throw\n";
    return false;
}

} // namespace

int main() {
    bool ok = true;

    const std::string fixture = "tests/fixtures/test_field.png";
    const std::string captured = "work/test-capture.png";

    gte::FakeCamera camera(fixture);
    ok &= expectTrue("Fake camera returns output path", camera.captureImage(captured) == captured);
    ok &= expectTrue("Fake camera writes captured image", std::filesystem::exists(captured));
    ok &= expectTrue(
        "Fake camera captured image size matches fixture",
        std::filesystem::file_size(captured) == std::filesystem::file_size(fixture));

    const gte::PlateSolution parsed_astrometry =
        gte::PlateSolver::parseSolutionText("Field center: (RA,Dec) = (83.82208333, -5.39111111) deg.");
    ok &= expectNear("Astrometry parser RA", parsed_astrometry.center.ra_deg, 83.82208333, 1.0e-8);
    ok &= expectNear("Astrometry parser Dec", parsed_astrometry.center.dec_deg, -5.39111111, 1.0e-8);

    const gte::PlateSolution parsed_wcs =
        gte::PlateSolver::parseSolutionText("CRVAL1 = 10.68470833\nCRVAL2 = 41.26875\n");
    ok &= expectNear("WCS parser RA", parsed_wcs.center.ra_deg, 10.68470833, 1.0e-8);
    ok &= expectNear("WCS parser Dec", parsed_wcs.center.dec_deg, 41.26875, 1.0e-8);

    gte::PlateSolver solver({
        .executable = "python3",
        .arguments = {"tests/fixtures/fake_plate_solver.py", "{image}"},
    });
    const gte::PlateSolution solved = solver.solve(captured);
    ok &= expectNear("Fake solver RA", solved.center.ra_deg, 83.82208333, 1.0e-8);
    ok &= expectNear("Fake solver Dec", solved.center.dec_deg, -5.39111111, 1.0e-8);

    gte::ZwoCamera zwo;
    ok &= expectThrowsRuntimeError("ZWO camera stub throws", [&zwo] {
        (void)zwo.captureImage("work/zwo-capture.png");
    });

    ok &= expectThrowsRuntimeError("Parser rejects missing solution", [] {
        (void)gte::PlateSolver::parseSolutionText("no solution here");
    });

    if (ok) {
        std::cout << "All plate solver tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
