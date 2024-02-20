#pragma once

#include "WCore/WCore.h"
#include "WAssets/WAsset.h"
#include "WStructs/WGeometryStructs.h"


WCLASS()
class WCORE_API WStaticModel : public WAsset
{
    WOBJECT_BODY(WStaticModel)

public:
    void SetModel(const WModelStruct& model);
    const WModelStruct& GetModel() const;

private:
    WModelStruct model_{};
};