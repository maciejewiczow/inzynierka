#ifndef MESH_HEADER_GUARD
#define MESH_HEADER_GUARD

#include "BasicLinearAlgebra.h"
#include "Tridiagonal.h"
#include "material.h"
#include "IntegrationPoints.h"

#define DBG_PRINT(x)

template<int nNodes>
class Mesh {
public:
    struct Node {
        float t, r;
    };

    Node nodes[nNodes];

    void generate(float t0, float elemSize) {
        // Serial << "Generating the mesh" << endl;

        float r = 0;
        for (int i = 0; i < nNodes; i++) {
            nodes[i].t = t0;
            nodes[i].r = r;

            // Serial << "Node " << i << ": " << "r = " << r << ", t = " << t0 << endl;

            r += elemSize;
        }
        // Serial << endl;
    }

    void integrateStep(float dTau, float r, float tAmbient, const Material& config) {
        TridiagMat<nNodes> H;
        BLA::Matrix<nNodes> P;

        H.Fill(0);
        P.Fill(0);

        for (int i = 0; i < nNodes-1; i++) {
            BLA::Matrix<2,2> Hlocal;
            BLA::Matrix<2> Plocal;
            Hlocal.Fill(0);
            Plocal.Fill(0);

            auto& node1 = nodes[i];
            auto& node2 = nodes[i+1];

            float dR = fabs(node1.r - node2.r);

            DBG_PRINT(dR);

            float alphaAir = (i == nNodes-2) ? config.alphaAir : 0;

            for (unsigned j = 0; j <= config.integrationScheme; j++) {
                const auto& intPoint = IntegrationPoints::get(config.integrationScheme, j);

                DBG_PRINT(intPoint.xi);
                DBG_PRINT(intPoint.weight);

                float n0 = 0.5f * (1 - intPoint.xi);
                float n1 = 0.5f * (1 + intPoint.xi);

                DBG_PRINT(n0);
                DBG_PRINT(n1);

                float r = node1.r * n0 + node2.r * n1;
                float t = node1.t * n0 + node2.t * n1;

                DBG_PRINT(r);
                DBG_PRINT(t);

                auto tmp = config.C * config.Ro * dR * r * intPoint.weight;

                Hlocal(0,0) += config.K*r*intPoint.weight/dR + tmp*n0*n0/dTau;
                Hlocal(0,1) += -config.K*r*intPoint.weight/dR + tmp*n0*n1/dTau;
                Hlocal(1,0) = Hlocal(0,1);
                Hlocal(1,1) += config.K*r*intPoint.weight/dR + tmp*n1*n1/dTau + 2.f*alphaAir*r;

                Plocal(0) += tmp*t*n0/dTau;
                Plocal(1) += tmp*t*n1/dTau + 2.f*alphaAir*r*tAmbient;
            }


            P.template Submatrix<2, 1>(i, 0) += Plocal;
            H.template Submatrix<2, 2>(i, i) += Hlocal;
        }

        // Serial << endl << "Global matrices calculated" << endl
        //          << "H:" << endl << H << endl
        //          << "P:" << endl << P << endl;

        auto decomposition = BLA::LUDecompose(H);
        auto x = BLA::LUSolve(decomposition, P);

        for (int i = 0; i < nNodes; i++)
            nodes[i].t = x(i);
    }
};

#endif
