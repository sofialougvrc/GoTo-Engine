#include "gte/plate_solver.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace gte {
namespace {

std::string shellQuote(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

std::string replaceImagePlaceholder(std::string value, const std::string& image_path) {
    constexpr const char* kPlaceholder = "{image}";
    std::size_t pos = 0;
    while ((pos = value.find(kPlaceholder, pos)) != std::string::npos) {
        value.replace(pos, std::char_traits<char>::length(kPlaceholder), image_path);
        pos += image_path.size();
    }
    return value;
}

PlateSolution parseRegex(
    const std::string& text,
    const std::regex& pattern,
    std::size_t ra_match,
    std::size_t dec_match) {
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        throw std::runtime_error("Plate solution pattern not found");
    }

    return {{
        std::stod(match[ra_match].str()),
        std::stod(match[dec_match].str()),
    }};
}

bool regexMatches(const std::string& text, const std::regex& pattern) {
    return std::regex_search(text, pattern);
}

} // namespace

PlateSolver::PlateSolver(PlateSolverCommand command)
    : command_(std::move(command)) {
    if (command_.executable.empty()) {
        throw std::invalid_argument("PlateSolver executable must not be empty");
    }
}

PlateSolution PlateSolver::solve(const std::string& image_path) const {
    const std::string command = buildCommand(image_path);
    std::array<char, 4096> buffer{};
    std::string output;

    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("Unable to run plate solver command");
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        output += buffer.data();
    }

    const int status = pclose(pipe.release());
    if (status != 0) {
        throw std::runtime_error("Plate solver command failed: " + output);
    }

    return parseSolutionText(output);
}

PlateSolution PlateSolver::parseSolutionText(const std::string& solver_output) {
    const std::string number = R"(([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?))";

    const std::regex astrometry_field_center(
        "Field center:\\s*\\(RA,Dec\\)\\s*=\\s*\\(\\s*" + number +
        "\\s*,\\s*" + number + "\\s*\\)\\s*deg",
        std::regex::icase);
    if (regexMatches(solver_output, astrometry_field_center)) {
        return parseRegex(solver_output, astrometry_field_center, 1, 2);
    }

    const std::regex key_value_ra_dec(
        "\\bRA\\s*=\\s*" + number + R"([\s\S]*?\bDEC\s*=\s*)" + number,
        std::regex::icase);
    if (regexMatches(solver_output, key_value_ra_dec)) {
        return parseRegex(solver_output, key_value_ra_dec, 1, 2);
    }

    const std::regex wcs_crval(
        R"(\bCRVAL1\s*=\s*)" + number + R"([\s\S]*?\bCRVAL2\s*=\s*)" + number,
        std::regex::icase);
    if (regexMatches(solver_output, wcs_crval)) {
        return parseRegex(solver_output, wcs_crval, 1, 2);
    }

    throw std::runtime_error("Unable to parse plate solver RA/Dec solution");
}

std::string PlateSolver::buildCommand(const std::string& image_path) const {
    std::string command = shellQuote(command_.executable);
    for (const auto& argument : command_.arguments) {
        command += ' ';
        command += shellQuote(replaceImagePlaceholder(argument, image_path));
    }
    return command;
}

} // namespace gte
