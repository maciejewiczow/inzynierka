#ifndef MESH_HEADER_GUARD
#define MESH_HEADER_GUARD

#include <BasicLinearAlgebra.h>
#include <KeepMeAlive.h>
#include "Tridiagonal.h"
#include "IntegrationPoints.h"
#include "print_util.h"

using namespace prnt;

class Input;

/*
    Represents the one-dimensional FEM mesh.
    Stores all the nodes and has methods that generate their coordinates
    and preform the integration.
*/
template<int nNodes>
class Mesh {
public:
    struct Node {
        float t, r;
    };

    Node nodes[nNodes];

    void generate(float t0, float elemSize) {
        float r = 0;
        for (auto& node : nodes) {
            node.t = t0;
            node.r = r;

            r += elemSize;
        }
    }

    void integrateStep(float dTau, float rMax, float tAmbient, const Input& input) {
        TridiagMat<nNodes> K;
        BLA::Matrix<nNodes> F;

        K.Fill(0);
        F.Fill(0);

        for (int i = 0; i < nNodes-1; i++) {
            BLA::Matrix<2,2> Klocal;
            BLA::Matrix<2> Flocal;
            Klocal.Fill(0);
            Flocal.Fill(0);

            auto& nodeI = nodes[i];
            auto& nodeJ = nodes[i+1];

            float dR = fabs(nodeI.r - nodeJ.r);

            float alphaAir = (i == nNodes-2) ? input.alphaAir : 0;

            for (unsigned j = 0; j <= input.integrationScheme; j++) {
                const auto& intPoint = IntegrationPoints::get(input.integrationScheme, j);

                float n0 = 0.5f * (1 - intPoint.xi);
                float n1 = 0.5f * (1 + intPoint.xi);

                float r = nodeI.r * n0 + nodeJ.r * n1;
                float t = nodeI.t * n0 + nodeJ.t * n1;

                auto tmp = input.C * input.Ro * dR * r * intPoint.weight;

                Klocal(0,0) += input.K*r*intPoint.weight/dR + tmp*n0*n0/dTau;
                Klocal(0,1) += -input.K*r*intPoint.weight/dR + tmp*n0*n1/dTau;
                Klocal(1,0) = Klocal(0,1);
                Klocal(1,1) += input.K*r*intPoint.weight/dR + tmp*n1*n1/dTau + 2.f*alphaAir*rMax;

                Flocal(0) += tmp*t*n0/dTau;
                Flocal(1) += tmp*t*n1/dTau + 2.f*alphaAir*rMax*tAmbient;
                watchdogTimer.reset();
            }

            F.template Submatrix<2, 1>(i, 0) += Flocal;
            K.template Submatrix<2, 2>(i, i) += Klocal;
        }

        auto decomposition = BLA::LUDecompose(K);
        auto t = BLA::LUSolve(decomposition, F);
        watchdogTimer.reset();

        for (int i = 0; i < nNodes; i++)
            nodes[i].t = t(i);
    }
};

#endif
