// ТАУ Лаба 3 — Система с разнотемповыми движениями
// Вариант 1: K2=2, T2=0.5, d=0.8, K3=3, eps=0.3
// Структура: v -> W1(eps*p) -> [+M] -> W2(p) -> W3(p) -> y
// W1 = 1/(eps^2*p^2 + 2*d*eps*p + 1)
// W2 = K2/(T2*p+1)
// W3 = K3/p
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>

// Variant 1 parameters
const double K2  = 2.0;
const double T2  = 0.5;
const double d   = 0.8;
const double K3  = 3.0;

// State: x1=y (out of W3), x2=out of W2, x3=out of W1, x4=x3'
struct State4 { double x1, x2, x3, x4; };

State4 deriv3(const State4& s, double eps, double v, double M_dist) {
    // W3: x1' = K3*x2
    double dx1 = K3 * s.x2;
    // W2: T2*x2' = -x2 + K2*(x3 + M_dist)
    double dx2 = (-s.x2 + K2*(s.x3 + M_dist)) / T2;
    // W1: eps^2*x3'' + 2*d*eps*x3' + x3 = v => x4 = x3', x4' = (v-x3-2d*eps*x4)/eps^2
    double dx3 = s.x4;
    double dx4 = (v - s.x3 - 2.0*d*eps*s.x4) / (eps*eps);
    return {dx1, dx2, dx3, dx4};
}

State4 rk4_3(const State4& s, double dt, double eps, double v, double M_dist) {
    auto f = [&](const State4& s_) { return deriv3(s_, eps, v, M_dist); };
    State4 k1 = f(s);
    State4 s2 = {s.x1+0.5*dt*k1.x1, s.x2+0.5*dt*k1.x2, s.x3+0.5*dt*k1.x3, s.x4+0.5*dt*k1.x4};
    State4 k2 = f(s2);
    State4 s3 = {s.x1+0.5*dt*k2.x1, s.x2+0.5*dt*k2.x2, s.x3+0.5*dt*k2.x3, s.x4+0.5*dt*k2.x4};
    State4 k3 = f(s3);
    State4 s4 = {s.x1+dt*k3.x1, s.x2+dt*k3.x2, s.x3+dt*k3.x3, s.x4+dt*k3.x4};
    State4 k4 = f(s4);
    return {
        s.x1 + dt/6*(k1.x1+2*k2.x1+2*k3.x1+k4.x1),
        s.x2 + dt/6*(k1.x2+2*k2.x2+2*k3.x2+k4.x2),
        s.x3 + dt/6*(k1.x3+2*k2.x3+2*k3.x3+k4.x3),
        s.x4 + dt/6*(k1.x4+2*k2.x4+2*k3.x4+k4.x4)
    };
}

void simulate3(const std::string& fname, double eps, double v0,
               double M_t_start, double T_end, double dt) {
    std::ofstream f(fname);
    f << "t,x1,x2,x3\n";
    State4 s = {0, 0, 0, 0};
    for (double t = 0; t <= T_end; t += dt) {
        double M_dist = (t >= M_t_start && M_t_start > 0) ? 1.0 : 0.0;
        f << t << "," << s.x1 << "," << s.x2 << "," << s.x3 << "\n";
        s = rk4_3(s, dt, eps, v0, M_dist);
    }
    f.close();
    std::cout << "Saved: " << fname << " (eps=" << eps << ")\n";
}

int main() {
    const double eps0 = 0.3;
    // dt must be < eps_min/10
    double dt = eps0 / 400.0;  // small enough for fast dynamics

    // 4.1: eps=0.3, v=1(t), M=0
    simulate3("exp1_eps03.csv", eps0, 1.0, -1, 10.0, dt);

    // 4.2: Reduce eps by 5, 10, 40 times
    simulate3("exp2_eps_5x.csv", eps0/5,  1.0, -1, 10.0, eps0/5/100);
    simulate3("exp3_eps_10x.csv", eps0/10, 1.0, -1, 5.0, eps0/10/100);
    simulate3("exp4_eps_40x.csv", eps0/40, 1.0, -1, 5.0, eps0/40/100);

    // 4.7: eps=eps0, v=0, M=1 applied at tau=1s
    simulate3("exp5_disturbance.csv", eps0, 0.0, 1.0, 10.0, dt);

    // Vary d (4.4): eps=eps0
    double d_vals[] = {0.1, 0.3, 0.5, 0.8, 1.2};
    std::ofstream fd("exp6_d_vary.csv");
    fd << "t";
    for (double dv : d_vals) fd << ",x1_d" << dv;
    fd << "\n";
    // Run all simultaneously
    std::vector<std::vector<double>> x1s(5);
    std::vector<State4> states(5, {0,0,0,0});
    int Nsteps = (int)(10.0/dt);
    double d_save = d;
    // Hack: run separately and merge
    fd.close();
    // Run each separately
    for (int di = 0; di < 5; di++) {
        std::string fname = "exp6_d" + std::to_string(di) + ".csv";
        // Need to use different d value — rewrite deriv with local d
        std::ofstream ff(fname);
        ff << "t,x1,x2,x3\n";
        State4 s = {0,0,0,0};
        double dv = d_vals[di];
        for (double t = 0; t <= 10.0; t += dt) {
            ff << t << "," << s.x1 << "," << s.x2 << "," << s.x3 << "\n";
            // Inline derivatives with custom d
            double dx1 = K3*s.x2;
            double dx2 = (-s.x2 + K2*s.x3)/T2;
            double dx3 = s.x4;
            double dx4 = (1.0 - s.x3 - 2.0*dv*eps0*s.x4)/(eps0*eps0);
            State4 k1 = {dx1,dx2,dx3,dx4};
            State4 s2 = {s.x1+0.5*dt*k1.x1,s.x2+0.5*dt*k1.x2,s.x3+0.5*dt*k1.x3,s.x4+0.5*dt*k1.x4};
            dx1=K3*s2.x2; dx2=(-s2.x2+K2*s2.x3)/T2; dx3=s2.x4; dx4=(1.0-s2.x3-2.0*dv*eps0*s2.x4)/(eps0*eps0);
            State4 k2 = {dx1,dx2,dx3,dx4};
            State4 s3 = {s.x1+0.5*dt*k2.x1,s.x2+0.5*dt*k2.x2,s.x3+0.5*dt*k2.x3,s.x4+0.5*dt*k2.x4};
            dx1=K3*s3.x2; dx2=(-s3.x2+K2*s3.x3)/T2; dx3=s3.x4; dx4=(1.0-s3.x3-2.0*dv*eps0*s3.x4)/(eps0*eps0);
            State4 k3 = {dx1,dx2,dx3,dx4};
            State4 s4 = {s.x1+dt*k3.x1,s.x2+dt*k3.x2,s.x3+dt*k3.x3,s.x4+dt*k3.x4};
            dx1=K3*s4.x2; dx2=(-s4.x2+K2*s4.x3)/T2; dx3=s4.x4; dx4=(1.0-s4.x3-2.0*dv*eps0*s4.x4)/(eps0*eps0);
            State4 k4 = {dx1,dx2,dx3,dx4};
            s = {s.x1+dt/6*(k1.x1+2*k2.x1+2*k3.x1+k4.x1),
                 s.x2+dt/6*(k1.x2+2*k2.x2+2*k3.x2+k4.x2),
                 s.x3+dt/6*(k1.x3+2*k2.x3+2*k3.x3+k4.x3),
                 s.x4+dt/6*(k1.x4+2*k2.x4+2*k3.x4+k4.x4)};
        }
        ff.close();
        std::cout << "Saved: " << fname << " (d=" << dv << ")\n";
    }

    std::cout << "Done.\n";
    return 0;
}
