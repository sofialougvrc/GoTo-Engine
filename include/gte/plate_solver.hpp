#pragma once

#include "gte/coord_transform.hpp"

#include <string>
#include <vector>

namespace gte {

struct PlateSolution {
    EquatorialCoord center;
};

struct PlateSolverCommand {
    std::string executable;
    std::vector<std::string> arguments;
};

class PlateSolver {
public:
    explicit PlateSolver(PlateSolverCommand command);

    PlateSolution solve(const std::string& image_path) const;

    static PlateSolution parseSolutionText(const std::string& solver_output);

private:
    std::string buildCommand(const std::string& image_path) const;

    PlateSolverCommand command_;
};

} // namespace gte
