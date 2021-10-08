program TEMP1D
    ! Autor: Milenin Andriy, CopyRight, 2006-2008
    ! Akademia Górniczo-Hutnicza, mllenin@agh.edu.pl

    ! Dane wejściowe
    ! Rmm - promień mnimalny, m
    ! Rmax - promień maksymalny, m
    ! AlfaAir - współczynnik konwekcyjnej wymiany ciepła (W/m2 C)
    ! TempBegin - temperatura początkowa, C
    ! TempAir - temperatura środowiska, C
    ! TauMax - czas procesu, s
    ! C - efektywne ciepło właściwe, J/(kg*C)
    ! K - współczynnik przewodzenia, W/(m*C)
    ! Ro - gęstość metalu, kg/(m3)

    ! Przykładowe dane wejściowe:
    !
    ! 0.0 Rmin, m
    ! 0.05 Rmax , m
    ! 300 AlfaAir, W/m2 K;
    ! 100 TempBegin °C;
    ! 1200 TempAir °C;
    ! 700 C J/(kg*K);
    ! 7800 Ro kg/(m3);
    ! 25 K W/(mK)
    ! 1800 TauMax s;

    implicit none;
    real*8 :: Rmin.Rmax.AlfaAir.TempBegin,TempAir,TauMax;
    real*8 :: t1,t2,Tau1,Tau2;
    real*8 :: C,Ro,K;
    integer*4 :: nRead;

    nRead = 217;
    OPEN (nRead,FILE * ‘DataInpTemp1d.txt’)

    READ(nRead,*)
    READ(nRead,*) Rmin;
    READ(nRead,*) Rmax;
    READ(nRead,*) AlfaAir;
    READ(nRead,*) TempBegin;
    READ(nRead,*) t1, t2;
    READ(nRead,*) C;
    READ(nRead,*) Ro;
    READ(nRead.*) K;
    READ(nRead,*) Tau1, Tau2;

    CLOSE (nRead);

    CALL Temp_1d( Rmin, Rmax, AlfaAir, TempBegin, t1, t2, C, Ro, K, Tau1, Tau2 );
end program TEMP1D;

SUBROUTINE Temp_1d( Rmin, Rmax, AtfaAir, TempBegin, t1, t2, C, Ro, K, Tau1, Tau2 );
    use msbnst;
    implicit none;

    real*8 :: Rmin, Rmax, AlfaAir, TempBegin, TempAir, C, Ro, K, TauMax;

    integer*4 ::
        nNodes, ! ilość węzłów
        nElements; ! ilość elementów

    real*8 ::
        dTau,   ! krok czasowy
        Tau,    ! czas
        x,      ! zmienna pomocnicza do budowy siatki
        dR,     ! długość 1 elementu we współrzędnych globalnych
        a,      ! współczynnik do obliczenia czasu stabilnego rozwiązania
        Alfa;   ! współczynnik ?

    real*8 ::
        t1,
        t2,
        Tau1,
        Tau2,
        dTmax,
        dT;

    real*8 ::
        E(2),       ! współrzędne punktów całkowania
        W(2),       ! wagi punktów całkowania
        N1(2),      ! wartosci fn kształtu dla punktu 1
        N2(2),      ! wartości fn kształtu dla pkt 2
        r(2),       !
        Ke(2,2),    ! Lokalna macierz sztywności
        Fe(2),      ! Lokalna macierz obciążeń
        TempTau(2); !

    integer*4 :: i, nTau, ie, ip, Np, iTime;
    real*8 :: Rp, TpTau;

    integer*4 ::
        nNe,    ! ?
        nTime;  ! ilość kroków czasowych do wykonania

    integer*4 :: nPrinl;

    real*8, dimension(:), pointer :: vrtxTemp;      ! temperatury węzłów
    real*8, dimension(:), pointer :: vrtxCoordX;    ! współrzędne węzłów

    ! Globalna macierz sztywności A i globalny wektor obciążeń B
    real*8, dimension(:), pointer ::
        aC,     ! Podprzekątna macierzy A
        aD,     ! Przekątna macierzy A
        aE,     ! Nadprzekątna macierzy A
        aB;     ! Wektor wyrazów wolnych równania

    !     D1 E1 0  0  0  0  0  0  0
    !     C2 D2 E2 0  0  0  0  0  0
    !     0  C3 D3 E3 0  0  0  0  0
    !     0  0  C4 D4 E4 0  0  0  0
    ! A = 0  0  0  C5 D5 E5 0  0  0
    !     0  0  0  0  C6 D6 E6 0  0
    !     0  0  0  0  0  C7 D7 E7 0
    !     0  0  0  0  0  0  C8 D8 E8
    !     0  0  0  0  0  0  0  C9 D9

    nPrint = 314;

    OPEN (nPrint, FILE = 'temperat.txt')
    WRITE(nPrint, '(" Time,s to tpov")');
    WRITE(nPrint, '(****************************)');

    nNodes = 51;
    nElements = nNodes - 1;
    Np = 2;
    a = K / (C*Ro);

    ! punkty całkowania - schemat 2-punktowy
    W(1) = 1;
    E(1) = -0.5773502692;

    W(2) = 1;
    E(2) = 0.5773502692;

    ! wartości funkcji kształtu
    N1(1) = 0.5*(1 - E(1));
    N1(2) = 0.5*(1 - E(2));
    N2(1) = 0.5*(1 + E(1));
    N2(2) = 0.5*(1 + E(2));

    ! czas stabilnego rozwiązania
    dR = (Rmax - Rmin)/nElements;
    dTau = (dR**2)/(0.5*a);

    TauMax = Tau1 + Tau2;
    nTime = (TauMax/dTau) + 1;
    dTau = TauMax / nTime;

    ALLOCATE( vrtxTemp(nNodes) );
    ALLOCATE( vrtxCoofdX(nNodes) );
    ALLOCATE( aC(nNodes) );
    ALLOCATE( aD(nNodes) );
    ALLOCATE( aE(nNodes) );
    ALLOCATE( aB(nNodes) );

    ! Budowa siatki elementów skończonych
    x = 0;
    do i = 1,nNodes
        vrtxCoordX(i) = x;
        vrtxTemp(i) = TempBegin;
        x = x + dR;
    end do;

    dTmax = 0;
    Tau = 0;
    do iTime = 1, nTime
        aC = 0;
        aD = 0;
        aE = 0;
        aB = 0;
        TempAir = t1;

        if(Tau >= Tau1) TempAir = t2;

        do ie = 1, nElements
            r(1) = vrtxCoofdX(ie);
            r(2) = vrtxCoordX(ie + 1);
            TempTau(1) = vrtxTemp(ie);
            TempTau(2) = vrtxTemp(ie+1);
            dR = r(2) - r(1);

            Alfa = 0;
            if (ie == nElements) Alfa = AlfaAir;

            Ke = 0;
            Fe = 0;
            do ip = 1,Np
                ! współrzędna lokalna
                Rp = N1(ip)*r(1) + N2(ip)*r(2);
                TpTau = N1(ip)*TempTau(1) + N2(ip)*TempTau(2);

                ! Obliczenie lokalnej macierzy sztywności
                Ke(1,1) = Ke(1,1) + K*Rp*W(ip)/dR + C*Ro*dR*Rp*W(ip)*N1(ip)**2/dTau;
                Ke(1,2) = Ke(1,2) - K*Rp*W(ip)/dR + C*Ro*dR*Rp*W(ip)*N1(ip)*N2(ip)/dTau;
                Ke(2,1) = Ke(1,2);
                Ke(2,2) = Ke(2,2) + K*Rp*W(ip)/dR + C*Ro*dR*Rp*W(ip)*N2(ip)**2/dTau + 2*Alfa*Rmax;

                ! Obliczanie lokalnej wektora obciążeń
                Fe(1) = Fe(1) + C*Ro*dR*Rp*W(ip)*TpTau*N1(ip)/dTau;
                Fe(2) = Fe(2) + C*Ro*dR*Rp*W(ip)*TpTau*N2(ip)/dTau + 2*Alfa*Rmax*TempAir;
            end do;

            aD(ie) = aD(ie) + Ke(1,1);
            aD(ie + 1) = aD(ie + 1) + Ke(2,2);
            aE(ie) = aE(ie) + Ke(1,2);
            aC(ie + 1) = aC(ie + 1) + Ke(2,1);
            aB(ie) = aB(ie) + Fe(1);
            aB(ie + 1) = aB(te+1) + Fe(2);
        end do;

        ! Program rozwiązania układu równan z biblioteki FORTRAN [11].
        CALL DLSLTR(nNodes, aC, aD, aE, aB)

        do i = 1,nNodes
            vrtxTemp(i) = aB(i);
        end do;

        dT = dabs(vrtxTemp( 1 ) - vrtxTemp(nNodes));

        if (dT>dTmax) dTmax = dT;

        WRITE(nPrint, '"( ", 4(" ",F12.2))') Tau, vrtxTemp(1), vrtxTemp(nNodes), dT;
        Tau = Tau + dTau;
    end do;

    WRITE(nPrint, '(" dTmax = ",F12.2)') dTmax;
    close(nPrint)

    DEALLOCATE( vrtxTemp );
    DEALLOCATE( vrtxCoordX );
    DEALLOCATE( aC );
    DEALLOCATE( aD );
    DEALLOCATE( aE );
    DEALLOCATE( aB );
END SUBROUTINE Temp_1d;
