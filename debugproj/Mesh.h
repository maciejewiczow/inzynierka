#ifndef MESH_HEADER_GUARD
#define MESH_HEADER_GUARD

#include "BasicLinearAlgebra.h"
#include "Tridiagonal.h"
#include "Config.h"
#include "IntegrationPoints.h"


// #define DBG_PRINT(x) std::cout << #x " = " << x << std::std::endl
#define DBG_PRINT(x)

template<int nNodes>
class Mesh {
public:
    struct Node {
        float t, x;

        using shapeFn = float (*)(float);

        inline static const shapeFn N[] = {
            [](float ksi) { return 0.5f*(1 - ksi); },
            [](float ksi) { return 0.5f*(1 + ksi); }
        };
    };

    Node nodes[nNodes];

    void generate(float t0, float elemSize) {
        std::cout << "Generating the mesh" << std::endl;

        float x = 0;
        for (int i = 0; i < nNodes; i++) {
            nodes[i].t = t0;
            nodes[i].x = x;

            std::cout << "Node " << i << ": " << "x = " << x << ", t = " << t0 << std::endl;

            x += elemSize;
        }
        std::cout << std::endl;
    }

    void integrateStep(float dTau, float r, float tAmbient, const Config& config) {
        std::cout << "\tIntegration start" << std::endl;

        MatTridiag<nNodes> H;
        BLA::Matrix<nNodes> P;

        H.Fill(0);
        P.Fill(0);

        for (int i = 0; i < nNodes-1; i++) {
            BLA::Matrix<2,2> Hlocal;
            BLA::Matrix<2> Plocal;
            Hlocal.Fill(0);
            Plocal.Fill(0);

            // wdt_reset();

            auto& node1 = nodes[i];
            auto& node2 = nodes[i+1];

            // std::cout << "\t\tProcessing nodes " << i << " and " << i+1 << std::endl;

            float dR = fabs(node1.x - node2.x);

            DBG_PRINT(dR);

            float alphaAir = (i == nNodes-2) ? config.alphaAir : 0;

            // wdt_reset();

            for (int j = 0; j <= config.integrationScheme; j++) {
                const auto& intPoint = IntegrationPoints::get(config.integrationScheme, j);

                // std::cout << "\t\t\t Processing integration point " << j << std::endl;

                DBG_PRINT(intPoint.x);
                DBG_PRINT(intPoint.weight);

                float n0 = Node::N[0](intPoint.x);
                float n1 = Node::N[1](intPoint.x);

                // wdt_reset();

                DBG_PRINT(n0);
                DBG_PRINT(n1);

                float x = node1.x * n0 + node2.x * n1;
                float t = node1.t * n0 + node2.t * n1;

                // wdt_reset();

                DBG_PRINT(x);
                DBG_PRINT(t);

                auto tmp = config.C * config.Ro * dR * x * intPoint.weight;

                Hlocal(0,0) += config.K*x*intPoint.weight/dR + tmp*n0*n0/dTau;
                // wdt_reset();
                Hlocal(0,1) += -config.K*x*intPoint.weight/dR + tmp*n0*n1/dTau;
                // wdt_reset();
                Hlocal(1,0) = Hlocal(0,1);
                // wdt_reset();
                Hlocal(1,1) += config.K*x*intPoint.weight/dR + tmp*n1*n1/dTau + 2.f*alphaAir*r;
                // wdt_reset();

                Plocal(0) += tmp*t*n0/dTau;
                // wdt_reset();
                Plocal(1) += tmp*t*n1/dTau + 2.f*alphaAir*r*tAmbient;
            }

            // std::cout << "\t\tLocal matrices calculated" << std::endl << "Hlocal = " << std::endl << Hlocal << std::endl << "Plocal = " << std::endl << Plocal << std::endl;

            P.template Submatrix<2, 1>(i, 0) += Plocal;
            // wdt_reset();
            H.template Submatrix<2, 2>(i, i) += Hlocal;
        }

        std::cout << std::endl << "Global matrices calculated" << std::endl
                 << "H:" << std::endl << H << std::endl
                 << "P:" << std::endl << P << std::endl;
        // wdt_reset();
        auto decomposition = BLA::LUDecompose(H);
        // wdt_reset();
        auto x = BLA::LUSolve(decomposition, P);
        // wdt_reset();

        // std::cout << "Solved equation system" << std::endl << x << std::endl;

        for (int i = 0; i < nNodes; i++)
            nodes[i].t = x(i);

        // std::cout << "Temps written" << std::endl;

        // wdt_reset();
    }
};

#endif
