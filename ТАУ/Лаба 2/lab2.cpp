// ТАУ Лаба 2 — Метод гармонического баланса
// Вариант 1: W_л(p) = 5 / (p*(p+1)*(0.5p+1))
// K=5, T1=1.0, T2=0.5
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <vector>
#include <functional>

const double M_nl = 1.0; // relay amplitude
const double a_nl = 1.0; // relay parameter (dead zone / hysteresis width)

// -------- Nonlinear elements --------
enum NLType { IDEAL_RELAY=1, RELAY_DEADZONE=2, RELAY_HYSTERESIS=3, DEADZONE=4 };

double nonlinearity(double e, NLType type, int& hyst_state) {
    switch(type) {
        case IDEAL_RELAY:
            return (e >= 0) ? M_nl : -M_nl;
        case RELAY_DEADZONE:
            if (std::abs(e) <= a_nl) return 0.0;
            return (e > 0) ? M_nl : -M_nl;
        case RELAY_HYSTERESIS:
            if (e > a_nl)  hyst_state = 1;
            if (e < -a_nl) hyst_state = -1;
            return hyst_state * M_nl;
        case DEADZONE:
            if (std::abs(e) <= a_nl) return 0.0;
            return e - a_nl * (e > 0 ? 1 : -1);
    }
    return 0.0;
}

// -------- State --------
struct State { double x1, x2, x3; }; // x1=y, x2=y', x3=y''

// W_л(p) = 5/(p*(p+1)*(0.5p+1))
// => y''' + 3y'' + 2y' = 10*u
// x1'= x2, x2'= x3, x3'= -2*x2 - 3*x3 + 10*u_nl
State derivatives(const State& s, double v, NLType type, int& hyst, double K_gain=1.0) {
    double e = v - s.x1;
    double u = nonlinearity(e, type, hyst) * K_gain;
    return { s.x2, s.x3, -2.0*s.x2 - 3.0*s.x3 + 10.0*u };
}

State rk4(const State& s, double dt, double v, NLType type, int& hyst, double K_gain=1.0) {
    auto f = [&](const State& s_) { return derivatives(s_, v, type, hyst, K_gain); };
    State k1 = f(s);
    State s2 = {s.x1+0.5*dt*k1.x1, s.x2+0.5*dt*k1.x2, s.x3+0.5*dt*k1.x3};
    State k2 = f(s2);
    State s3 = {s.x1+0.5*dt*k2.x1, s.x2+0.5*dt*k2.x2, s.x3+0.5*dt*k2.x3};
    State k3 = f(s3);
    State s4 = {s.x1+dt*k3.x1, s.x2+dt*k3.x2, s.x3+dt*k3.x3};
    State k4 = f(s4);
    return {
        s.x1 + dt/6*(k1.x1+2*k2.x1+2*k3.x1+k4.x1),
        s.x2 + dt/6*(k1.x2+2*k2.x2+2*k3.x2+k4.x2),
        s.x3 + dt/6*(k1.x3+2*k2.x3+2*k3.x3+k4.x3)
    };
}

void simulate(const std::string& fname, NLType type, double v,
              State x0, double T_end, double dt, int& hyst, double K_gain=1.0) {
    std::ofstream f(fname);
    f << "t,y,ydot\n";
    State s = x0;
    for (double t = 0; t <= T_end; t += dt) {
        f << t << "," << s.x1 << "," << s.x2 << "\n";
        s = rk4(s, dt, v, type, hyst, K_gain);
    }
    f.close();
    std::cout << "Saved: " << fname << "\n";
}

// Compute W_л(jω) = 5/(jω*(jω+1)*(0.5jω+1))
// Returns real and imaginary parts
void Wl(double w, double& re, double& im) {
    // denominator: p*(T1p+1)*(T2p+1) at p=jw
    // = jw*(jw+1)*(0.5jw+1)
    // (jw+1)*(0.5jw+1) = -0.5w^2 + 1.5jw + 1 = (1-0.5w^2) + j*1.5w
    // * jw = jw*(1-0.5w^2) - 1.5w^2 = -1.5w^2 + jw*(1-0.5w^2)
    double den_re = -1.5*w*w;
    double den_im = w*(1.0 - 0.5*w*w);
    double den_mag2 = den_re*den_re + den_im*den_im;
    // W = 5 / (den_re + j*den_im) = 5*(den_re - j*den_im)/mag2
    re = 5.0*den_re / den_mag2;
    im = -5.0*den_im / den_mag2;
}

void goldfarb_analysis() {
    std::ofstream f("goldfarb.csv");
    f << "w,Wl_re,Wl_im\n";
    // AFH of linear part
    for (double w = 0.01; w <= 5.0; w += 0.01) {
        double re, im;
        Wl(w, re, im);
        f << w << "," << re << "," << im << "\n";
    }
    f.close();

    // Find crossover with real axis: Im(Wl)=0 => w*(1-0.5w^2)=0 => w=sqrt(2)
    double w0 = std::sqrt(2.0);
    double re0, im0;
    Wl(w0, re0, im0);
    std::cout << "Goldfarb: w0=" << w0 << " rad/s\n";
    std::cout << "W_l(jw0) = " << re0 << " + j*" << im0 << "\n";
    // -1/W_n = -pi*A/4 = re0 => A = -4*re0/pi
    double A0 = -4.0*re0 / M_PI;
    std::cout << "Ideal relay: A0=" << A0 << ", w0=" << w0 << "\n";

    // Negative inverse characteristic for ideal relay
    std::ofstream f2("neg_inv_relay.csv");
    f2 << "A,neg_inv_re\n";
    for (double A = 0.1; A <= 10.0; A += 0.05) {
        double neg_inv = -M_PI*A / (4.0*M_nl);
        f2 << A << "," << neg_inv << "\n";
    }
    f2.close();
    std::cout << "Saved goldfarb.csv and neg_inv_relay.csv\n";
}

int main() {
    double dt = 0.001;
    int hyst = 1;

    // 4.2: Ideal relay, v=0, y(0)=2 — auto-oscillations
    hyst = 1;
    simulate("exp1_v0_relay.csv", IDEAL_RELAY, 0.0, {2.0,0,0}, 40.0, dt, hyst);

    // 4.4: Step response v=1(t), y(0)=0
    hyst = 1;
    simulate("exp2_step_relay.csv", IDEAL_RELAY, 1.0, {0,0,0}, 30.0, dt, hyst);

    // 4.5: Relay with dead zone, v=0, y(0)=2
    hyst = 1;
    simulate("exp3_deadzone_relay.csv", RELAY_DEADZONE, 0.0, {2.0,0,0}, 40.0, dt, hyst);

    // 4.6a: Relay with hysteresis, v=0, y(0)=2
    hyst = 1;
    simulate("exp4_hysteresis_relay.csv", RELAY_HYSTERESIS, 0.0, {2.0,0,0}, 40.0, dt, hyst);

    // 4.6b: Step response with hysteresis relay
    hyst = 1;
    simulate("exp5_step_hysteresis.csv", RELAY_HYSTERESIS, 1.0, {0,0,0}, 30.0, dt, hyst);

    // 4.6c: Dead zone nonlinearity — K nominal (converging)
    hyst = 1;
    simulate("exp6_dz_K1.csv", DEADZONE, 0.0, {2.0,0,0}, 40.0, dt, hyst, 1.0);
    // Larger K — diverging
    hyst = 1;
    simulate("exp7_dz_K3.csv", DEADZONE, 0.0, {2.0,0,0}, 40.0, dt, hyst, 3.0);

    goldfarb_analysis();
    return 0;
}
