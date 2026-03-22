// ТАУ Лаба 4 — Система, основанная на методе локализации
// Вариант 1: a1=2, a2=1, b=0.6, tp=3с, sigma=30%
// Объект: y'' + a2*y' + a1*y^2 = b*U - M(t)
// Управление: U = K*(F - y''_hat), F = -c1*y - c2*y' + c1*v
// Фильтр 2-го порядка: tau^2*z'' + 2*df*tau*z' + z = y
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>

// Variant 1
const double a1 = 2.0;
const double a2 = 1.0;
const double b  = 0.6;
// Design: tp=3s, sigma=30%
// From quality specs:
// xi = -ln(sigma/100)/sqrt(pi^2 + ln^2(sigma/100))
// sigma=0.3 => ln(0.3)=-1.204, xi=1.204/sqrt(9.87+1.449)=0.358
// w0 = pi/(tp*sqrt(1-xi^2)) = pi/(3*0.934)=1.12 rad/s
const double xi = 0.358;
const double w0 = 1.12;
const double c1 = w0*w0;     // ~1.254
const double c2 = 2*xi*w0;   // ~0.803

// Filter parameters — "fast" filter: tau = 0.1*T* = 0.1/w0
// T* = 1/w0 (desired time constant)
const double tau_fast = 0.1 / w0;  // ~0.089
const double df = 0.7;              // filter damping

// State: x1=y, x2=y', z1=y_hat (filter), z2=y'_hat
struct State6 { double x1, x2, z1, z2; };

double control_law(double z1, double z2, double z2dot, double v, double K) {
    double F = -c1*z1 - c2*z2 + c1*v;
    return K * (F - z2dot);
}

State6 deriv4(const State6& s, double v, double K, double M_dist, double tau) {
    // z2dot = filter 2nd deriv estimate
    double z2dot = (s.x1 - s.z1 - 2.0*df*tau*s.z2) / (tau*tau);
    double U = control_law(s.z1, s.z2, z2dot, v, K);
    // Object: x1'=x2, x2' = b*U - M - a2*x2 - a1*x1^2
    double dx1 = s.x2;
    double dx2 = b*U - M_dist - a2*s.x2 - a1*s.x1*s.x1;
    // Filter: dz1=z2, dz2=z2dot
    double dz1 = s.z2;
    double dz2 = z2dot;
    return {dx1, dx2, dz1, dz2};
}

State6 rk4_4(const State6& s, double dt, double v, double K, double M_dist, double tau) {
    auto f = [&](const State6& s_) { return deriv4(s_, v, K, M_dist, tau); };
    State6 k1 = f(s);
    State6 s2 = {s.x1+0.5*dt*k1.x1,s.x2+0.5*dt*k1.x2,s.z1+0.5*dt*k1.z1,s.z2+0.5*dt*k1.z2};
    State6 k2 = f(s2);
    State6 s3 = {s.x1+0.5*dt*k2.x1,s.x2+0.5*dt*k2.x2,s.z1+0.5*dt*k2.z1,s.z2+0.5*dt*k2.z2};
    State6 k3 = f(s3);
    State6 s4 = {s.x1+dt*k3.x1,s.x2+dt*k3.x2,s.z1+dt*k3.z1,s.z2+dt*k3.z2};
    State6 k4 = f(s4);
    return {
        s.x1 + dt/6*(k1.x1+2*k2.x1+2*k3.x1+k4.x1),
        s.x2 + dt/6*(k1.x2+2*k2.x2+2*k3.x2+k4.x2),
        s.z1 + dt/6*(k1.z1+2*k2.z1+2*k3.z1+k4.z1),
        s.z2 + dt/6*(k1.z2+2*k2.z2+2*k3.z2+k4.z2)
    };
}

void simulate4(const std::string& fname, double v, double K,
               double M_t_start, double T_end, double dt, double tau, bool log_U=false) {
    std::ofstream f(fname);
    f << "t,y,ydot,y_hat,ydot_hat" << (log_U ? ",U" : "") << "\n";
    State6 s = {0,0,0,0};
    for (double t = 0; t <= T_end; t += dt) {
        double M_dist = (M_t_start > 0 && t >= M_t_start) ? 1.0 : 0.0;
        // Compute U for logging
        double z2dot = (s.x1 - s.z1 - 2.0*df*tau*s.z2) / (tau*tau);
        double U = control_law(s.z1, s.z2, z2dot, v, K);
        f << t << "," << s.x1 << "," << s.x2 << "," << s.z1 << "," << s.z2;
        if (log_U) f << "," << U;
        f << "\n";
        s = rk4_4(s, dt, v, K, M_dist, tau);
    }
    f.close();
    std::cout << "Saved: " << fname << " K=" << K << " tau=" << tau << "\n";
}

// Open-loop object response (U=const=1)
void open_loop(const std::string& fname) {
    std::ofstream f(fname);
    f << "t,y,ydot\n";
    double x1=0, x2=0, dt=0.001;
    double U=1;
    for (double t=0; t<=20.0; t+=dt) {
        f << t << "," << x1 << "," << x2 << "\n";
        double dx1 = x2;
        double dx2 = b*U - a2*x2 - a1*x1*x1;
        // RK4
        double k1x1=x2, k1x2=b*U-a2*x2-a1*x1*x1;
        double x1b=x1+0.5*dt*k1x1, x2b=x2+0.5*dt*k1x2;
        double k2x1=x2b, k2x2=b*U-a2*x2b-a1*x1b*x1b;
        double x1c=x1+0.5*dt*k2x1, x2c=x2+0.5*dt*k2x2;
        double k3x1=x2c, k3x2=b*U-a2*x2c-a1*x1c*x1c;
        double x1d=x1+dt*k3x1, x2d=x2+dt*k3x2;
        double k4x1=x2d, k4x2=b*U-a2*x2d-a1*x1d*x1d;
        x1 += dt/6*(k1x1+2*k2x1+2*k3x1+k4x1);
        x2 += dt/6*(k1x2+2*k2x2+2*k3x2+k4x2);
    }
    f.close();
    std::cout << "Saved: " << fname << "\n";
}

int main() {
    // From bK = 20..100, choose K so that bK=30 => K=30/b=50
    double K_nom = 30.0 / b;  // ~50
    double dt = tau_fast / 20.0;  // small step for fast filter

    std::cout << "Design: xi=" << xi << " w0=" << w0 << " c1=" << c1 << " c2=" << c2 << "\n";
    std::cout << "K_nom=" << K_nom << " tau_fast=" << tau_fast << "\n";

    // 4.1: Open-loop step response (U=1)
    open_loop("exp1_openloop.csv");

    // 4.2: Fast filter, v=1, K=K_nom, M=0
    simulate4("exp2_fast_filter.csv", 1.0, K_nom, -1, 20.0, dt, tau_fast, true);

    // 4.4: Vary K: 1, 10, 50, 100
    double K_vals[] = {1.0, 10.0, K_nom, 100.0};
    for (double Kv : K_vals) {
        std::string fname = "exp3_K" + std::to_string((int)Kv) + ".csv";
        simulate4(fname, 1.0, Kv, -1, 20.0, dt, tau_fast, true);
    }

    // 4.5: Disturbance M=10*1(t-1)
    // Scale M by 10 — modify simulation: M_dist multiplied
    // Rerun with M_amp=10
    {
        std::ofstream f("exp4_disturbance.csv");
        f << "t,y,ydot,U\n";
        State6 s = {0,0,0,0};
        double M_amp = 10.0;
        for (double t = 0; t <= 20.0; t += dt) {
            double M_dist = (t >= 1.0) ? M_amp : 0.0;
            double z2dot = (s.x1 - s.z1 - 2.0*df*tau_fast*s.z2) / (tau_fast*tau_fast);
            double U = control_law(s.z1, s.z2, z2dot, 1.0, K_nom);
            f << t << "," << s.x1 << "," << s.x2 << "," << U << "\n";
            // Manual RK4 with M_dist at current time only (approximate)
            State6 d = deriv4(s, 1.0, K_nom, M_dist, tau_fast);
            State6 s2 = {s.x1+0.5*dt*d.x1,s.x2+0.5*dt*d.x2,s.z1+0.5*dt*d.z1,s.z2+0.5*dt*d.z2};
            State6 d2 = deriv4(s2,1.0,K_nom,M_dist,tau_fast);
            State6 s3 = {s.x1+0.5*dt*d2.x1,s.x2+0.5*dt*d2.x2,s.z1+0.5*dt*d2.z1,s.z2+0.5*dt*d2.z2};
            State6 d3 = deriv4(s3,1.0,K_nom,M_dist,tau_fast);
            State6 s4 = {s.x1+dt*d3.x1,s.x2+dt*d3.x2,s.z1+dt*d3.z1,s.z2+dt*d3.z2};
            State6 d4 = deriv4(s4,1.0,K_nom,M_dist,tau_fast);
            s = {s.x1+dt/6*(d.x1+2*d2.x1+2*d3.x1+d4.x1),
                 s.x2+dt/6*(d.x2+2*d2.x2+2*d3.x2+d4.x2),
                 s.z1+dt/6*(d.z1+2*d2.z1+2*d3.z1+d4.z1),
                 s.z2+dt/6*(d.z2+2*d2.z2+2*d3.z2+d4.z2)};
        }
        f.close();
        std::cout << "Saved: exp4_disturbance.csv\n";
    }

    // 4.7: Slow filter
    // Slow filter: bK=1+bK => tau0 = tau/sqrt(1+bK), d0=d/sqrt(1+bK)
    // From conditions: tau_slow = tau * sqrt(1+bK)... actually:
    // "medleniy filter" means tau chosen from separation condition: tau0 ≈ 0.1*T*
    // where T* is SLOW, i.e., T*_slow = T* * 10
    double tau_slow = tau_fast * 3.0;  // slower filter
    simulate4("exp5_slow_filter.csv", 1.0, K_nom, -1, 20.0, tau_slow/20.0, tau_slow, true);

    // 4.8: Nonlinear element "limitation" (saturation) on U
    {
        double U_limits[] = {5.0, 10.0, 20.0, 50.0};
        for (double Ulim : U_limits) {
            std::ofstream f("exp6_Ulim" + std::to_string((int)Ulim) + ".csv");
            f << "t,y,U\n";
            State6 s = {0,0,0,0};
            for (double t=0; t<=20.0; t+=dt) {
                double z2dot = (s.x1 - s.z1 - 2.0*df*tau_fast*s.z2)/(tau_fast*tau_fast);
                double U_raw = control_law(s.z1, s.z2, z2dot, 1.0, K_nom);
                double U = std::max(-Ulim, std::min(Ulim, U_raw));
                f << t << "," << s.x1 << "," << U << "\n";
                // Use saturated U in object
                double dx1=s.x2, dx2=b*U-a2*s.x2-a1*s.x1*s.x1;
                double dz1=s.z2, dz2=(s.x1-s.z1-2*df*tau_fast*s.z2)/(tau_fast*tau_fast);
                State6 d1={dx1,dx2,dz1,dz2};
                State6 s2={s.x1+0.5*dt*d1.x1,s.x2+0.5*dt*d1.x2,s.z1+0.5*dt*d1.z1,s.z2+0.5*dt*d1.z2};
                double U2=std::max(-Ulim,std::min(Ulim,control_law(s2.z1,s2.z2,(s2.x1-s2.z1-2*df*tau_fast*s2.z2)/(tau_fast*tau_fast),1.0,K_nom)));
                double dx1b=s2.x2,dx2b=b*U2-a2*s2.x2-a1*s2.x1*s2.x1,dz1b=s2.z2,dz2b=(s2.x1-s2.z1-2*df*tau_fast*s2.z2)/(tau_fast*tau_fast);
                State6 d2={dx1b,dx2b,dz1b,dz2b};
                State6 s3={s.x1+0.5*dt*d2.x1,s.x2+0.5*dt*d2.x2,s.z1+0.5*dt*d2.z1,s.z2+0.5*dt*d2.z2};
                double U3=std::max(-Ulim,std::min(Ulim,control_law(s3.z1,s3.z2,(s3.x1-s3.z1-2*df*tau_fast*s3.z2)/(tau_fast*tau_fast),1.0,K_nom)));
                double dx1c=s3.x2,dx2c=b*U3-a2*s3.x2-a1*s3.x1*s3.x1,dz1c=s3.z2,dz2c=(s3.x1-s3.z1-2*df*tau_fast*s3.z2)/(tau_fast*tau_fast);
                State6 d3={dx1c,dx2c,dz1c,dz2c};
                State6 s4={s.x1+dt*d3.x1,s.x2+dt*d3.x2,s.z1+dt*d3.z1,s.z2+dt*d3.z2};
                double U4=std::max(-Ulim,std::min(Ulim,control_law(s4.z1,s4.z2,(s4.x1-s4.z1-2*df*tau_fast*s4.z2)/(tau_fast*tau_fast),1.0,K_nom)));
                double dx1d=s4.x2,dx2d=b*U4-a2*s4.x2-a1*s4.x1*s4.x1,dz1d=s4.z2,dz2d=(s4.x1-s4.z1-2*df*tau_fast*s4.z2)/(tau_fast*tau_fast);
                State6 d4={dx1d,dx2d,dz1d,dz2d};
                s={s.x1+dt/6*(d1.x1+2*d2.x1+2*d3.x1+d4.x1),
                   s.x2+dt/6*(d1.x2+2*d2.x2+2*d3.x2+d4.x2),
                   s.z1+dt/6*(d1.z1+2*d2.z1+2*d3.z1+d4.z1),
                   s.z2+dt/6*(d1.z2+2*d2.z2+2*d3.z2+d4.z2)};
            }
            f.close();
            std::cout << "Saved exp6_Ulim" << (int)Ulim << ".csv\n";
        }
    }

    std::cout << "Done.\n";
    return 0;
}
