/*
 * Copyright 2020 Side Effects Software Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GEO_HAPI_PART_H__
#define __GEO_HAPI_PART_H__

#include "GEO_FilePrim.h"
#include "GEO_FilePrimUtils.h"
#include "GEO_HAPIAttribute.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormat.h"
#include <UT/UT_StringMap.h>
#include <UT/UT_UniquePtr.h>

PXR_NAMESPACE_USING_DIRECTIVE

class GEO_HAPIPart;
struct GEO_HAPIPrimCounts;

typedef UT_Array<GEO_HAPIPart> GEO_HAPIPartArray;

/// \class GEO_HAPIPart
///
/// Reads and stores Houdini Engine Primitive Data.
/// It also contains functions for converting its data
/// to USD.
///
class GEO_HAPIPart
{
public:
    GEO_HAPIPart();
    ~GEO_HAPIPart();

    // Copy constructor so this may be stored in UT_Arrays
    // Do not use this
    explicit GEO_HAPIPart(const GEO_HAPIPart &part) {}

    bool loadPartData(const HAPI_Session &session,
                      HAPI_GeoInfo &geo,
                      HAPI_PartInfo &part,
                      UT_WorkBuffer &buf);

    UT_BoundingBoxR getBounds();
    UT_Matrix4D getXForm();

    HAPI_PartType getType() { return myType; }
    bool isInstancer() { return myType == HAPI_PARTTYPE_INSTANCER; }

    // USD Functions

    static void partToPrim(GEO_HAPIPart &part,
                           const GEO_ImportOptions &options,
                           const SdfPath &parentPath,
                           GEO_FilePrimMap &filePrimMap,
                           const std::string &pathName,
                           GEO_HAPIPrimCounts &counts);

    // Returns false if the prim is undefined and
    // no more work should be done on it
    bool setupPrimType(GEO_FilePrim &filePrim,
                       GEO_FilePrimMap &filePrimMap,
                       const GEO_ImportOptions &options,
                       const std::string &pathName,
                       GT_DataArrayHandle &vertexIndirect);

    void setupPrimAttributes(GEO_FilePrim &filePrim,
                             const GEO_ImportOptions &options,
                             const GT_DataArrayHandle &vertexIndirect);

private:
    // Geometry metadata structs

    // Parent struct
    struct PartData
    {
        virtual ~PartData() = default;
    };

    struct CurveData : PartData
    {
        HAPI_CurveType curveType = HAPI_CURVETYPE_INVALID;
        GT_DataArrayHandle curveCounts;
        bool periodic = false;

        // Will be 0 when order is varying
        // and the constant value otherwise
        int constantOrder = 0;
        GT_DataArrayHandle curveOrders;

        // This may be empty after loading the part
        GT_DataArrayHandle curveKnots;
    };

    struct InstanceData : PartData
    {
	// instanceTransforms[i] is the transform of instances[i]
        GEO_HAPIPartArray instances;
        UT_Matrix4DArray instanceTransforms;
    };

    struct MeshData : PartData
    {
        GT_DataArrayHandle faceCounts;
        GT_DataArrayHandle vertices;
    };

    struct SphereData : PartData
    {
        UT_Vector3F center;
        float radius = 0.f;
    };

    struct VolumeData : PartData
    {
        std::string name;
        HAPI_VolumeType volumeType = HAPI_VOLUMETYPE_INVALID;

        UT_BoundingBoxF bbox;
        UT_Matrix4F xform;
    };

    bool checkAttrib(const UT_StringHolder &attribName,
                     const GEO_ImportOptions &options);

    // Modifies part to display cubic curves if they exist.
    // This is useful for when supported and unsupported curves
    // are attached to the same part
    void extractCubicBasisCurves();

    // Instancers hold attributes for their instances
    // When an instancer calls this, partOut will be filled
    // with data for a single instance
    void createInstancePart(GEO_HAPIPart &partOut, exint attribIndex);

    // USD Functions

    void setupInstances(const SdfPath &parentPath,
                        GEO_FilePrimMap &filePrimMap,
                        const std::string &pathName,
                        const GEO_ImportOptions &options,
                        GEO_HAPIPrimCounts &counts);

    void setupBoundsAttribute(GEO_FilePrim &filePrim,
                              const GEO_ImportOptions &options,
                              const GT_DataArrayHandle &vertexIndirect,
                              UT_ArrayStringSet &processedAttribs);

    void setupColorAttributes(GEO_FilePrim &filePrim,
                              const GEO_ImportOptions &options,
                              const GT_DataArrayHandle &vertexIndirect,
                              UT_ArrayStringSet &processedAttribs);

    void setupCommonAttributes(GEO_FilePrim &filePrim,
                               const GEO_ImportOptions &options,
                               const GT_DataArrayHandle &vertexIndirect,
                               UT_ArrayStringSet &processedAttribs);

    void setupMotionAttributes(GEO_FilePrim &filePrim,
                               const GEO_ImportOptions &options,
                               const GT_DataArrayHandle &vertexIndirect,
                               UT_ArrayStringSet &processedAttribs);

    void setupVisibilityAttribute(GEO_FilePrim &filePrim,
                                  const GEO_ImportOptions &options,
                                  UT_ArrayStringSet &processedAttribs);

    void setupExtraPrimAttributes(GEO_FilePrim &filePrim,
                                  UT_ArrayStringSet &processedAttribs,
                                  const GEO_ImportOptions &options,
                                  const GT_DataArrayHandle &vertexIndirect);

    void setupPointSizeAttribute(GEO_FilePrim &filePrim,
                                 const GEO_ImportOptions &options,
                                 const GT_DataArrayHandle &vertexIndirect,
                                 UT_ArrayStringSet &processedAttribs);

    template <class DT, class ComponentDT = DT>
    GEO_FileProp *applyAttrib(
        GEO_FilePrim &filePrim,
        const GEO_HAPIAttributeHandle &attrib,
        const TfToken &usdAttribName,
        const SdfValueTypeName &usdTypeName,
        UT_ArrayStringSet &processedAttribs,
        bool createIndicesAttrib,
        const GEO_ImportOptions &options,
        const GT_DataArrayHandle &vertexIndirect,
        const GT_DataArrayHandle &attribDataOverride = GT_DataArrayHandle());

    void convertExtraAttrib(GEO_FilePrim &filePrim,
                            GEO_HAPIAttributeHandle &attrib,
                            const TfToken &usdAttribName,
                            UT_ArrayStringSet &processedAttribs,
                            bool createIndicesAttrib,
                            const GEO_ImportOptions &options,
                            const GT_DataArrayHandle &vertexIndirect);

    HAPI_PartType myType;
    UT_StringArray myAttribNames;
    UT_StringMap<GEO_HAPIAttributeHandle> myAttribs;

    // This can be a PartData or any inheriting struct
    // The actual type of myData can be determined with myType
    typedef UT_UniquePtr<PartData> PartDataHandle;
    PartDataHandle myData;
};

#endif // __GEO_HAPI_PART_H__