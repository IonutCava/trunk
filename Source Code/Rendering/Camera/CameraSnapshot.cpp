#include "stdafx.h"

#include "Headers/CameraSnapshot.h"

namespace Divide {

bool operator==(const CameraSnapshot& lhs, const CameraSnapshot& rhs) {
    return lhs._FoV == rhs._FoV &&
           lhs._aspectRatio == rhs._aspectRatio &&
           lhs._zPlanes == rhs._zPlanes &&
           lhs._eye == rhs._eye &&
           lhs._viewMatrix == rhs._viewMatrix &&
           lhs._projectionMatrix == rhs._projectionMatrix &&
           lhs._orientation == rhs._orientation;
}

bool operator!=(const CameraSnapshot& lhs, const CameraSnapshot& rhs) {
    return lhs._FoV != rhs._FoV ||
           lhs._aspectRatio != rhs._aspectRatio ||
           lhs._zPlanes != rhs._zPlanes ||
           lhs._eye != rhs._eye ||
           lhs._viewMatrix != rhs._viewMatrix ||
           lhs._projectionMatrix != rhs._projectionMatrix ||
           lhs._orientation != rhs._orientation;
}
}; //namespace Divide