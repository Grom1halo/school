import sys
import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from scipy import signal
from scipy.optimize import minimize
from scipy.integrate import solve_ivp

# ── encoding ──────────────────────────────────────────────────────────────────
sys.stdout.reconfigure(encoding='utf-8')
sys.stderr.reconfigure(encoding='utf-8')

PLOTS_DIR = r"D:\school\ТАУ\Курсовая\plots"
os.makedirs(PLOTS_DIR, exist_ok=True)

plt.rcParams.update({
    'font.size': 11,
    'axes.titlesize': 12,
    'axes.labelsize': 11,
    'legend.fontsize': 10,
    'figure.dpi': 150,
    'axes.grid': True,
    'grid.alpha': 0.4,
})

# ══════════════════════════════════════════════════════════════════════════════
# PLANT PARAMETERS  (variant 5)
# ══════════════════════════════════════════════════════════════════════════════
R1  = 50.0          # Ом  — сопротивление якорной цепи
T1  = 0.4           # с   — постоянная времени якорной цепи
L   = T1 * R1       # Гн  — индуктивность якоря  (= 20 Гн)
C1  = 11.0          # В·с/рад — ДЭДС
C2  = 11.0          # Нм/А    — момент
J   = 0.06          # кг·м²
k1  = 5.3e-3        # м   — коэффициент линейной скорости (S = k1·ω)
k2  = 1.3e8         # Н/м — коэффициент мощности резания (N1 = k2·S·h)
K3  = 0.003         # с   — постоянная времени датчика
T2  = 0.07          # с   — электромеханическая постоянная привода шлифовального круга
Ku  = 13.64e-3      # 1/А — коэффициент датчика

# Электромеханическая постоянная двигателя
Tm  = J * R1 / (C1 * C2)      # ≈ 0.02479 с

# Требования
tp_req   = 0.6      # с   — время переходного процесса
sigma_req = 0.006   # 6‰  = 0.6 %
h_min  = 70e-6      # м
h_nom  = 125e-6     # м   — номинальное (середина диапазона)
h_max  = 180e-6     # м   — наихудший случай (расчётный)

# ── Статический коэффициент объекта ─────────────────────────────────────────
def K_obj(h):
    return k1 * k2 * h / C1

K_max = K_obj(h_max)
K_nom = K_obj(h_nom)
K_min = K_obj(h_min)

print(f"Tm  = {Tm:.5f} с")
print(f"K_obj(h_max) = {K_max:.4f}")
print(f"K_obj(h_nom) = {K_nom:.4f}")
print(f"K_obj(h_min) = {K_min:.4f}")

# ══════════════════════════════════════════════════════════════════════════════
# TRANSFER FUNCTION OF THE PLANT
#   W_obj(p) = K / [(T1·Tm·p² + Tm·p + 1)·(T2·p + 1)]
# ══════════════════════════════════════════════════════════════════════════════
def plant_tf(h):
    K  = K_obj(h)
    # числитель: K
    num = [K]
    # знаменатель: (T1·Tm·p² + Tm·p + 1)·(T2·p + 1)
    d1 = [T1 * Tm, Tm, 1.0]       # квадратный полином
    d2 = [T2, 1.0]                 # первый порядок
    den = np.polymul(d1, d2)       # перемножить
    return signal.TransferFunction(num, den)

# Датчик: W_sens(p) = Ku / (K3·p + 1)
sens_tf = signal.TransferFunction([Ku], [K3, 1.0])

# ══════════════════════════════════════════════════════════════════════════════
# 1. ДИАГРАММЫ БОДЕ разомкнутого объекта
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 1. Диаграммы Боде объекта управления ===")

w = np.logspace(-2, 3, 2000)

fig, axes = plt.subplots(2, 1, figsize=(10, 7))
fig.suptitle('Диаграммы Боде объекта управления (разомкнутая система)\nВариант 5 — Система стабилизации мощности резания')

colors = {'h_min': 'tab:blue', 'h_nom': 'tab:green', 'h_max': 'tab:red'}
labels_h = {
    'h_min': f'h = {h_min*1e6:.0f} мкм',
    'h_nom': f'h = {h_nom*1e6:.0f} мкм',
    'h_max': f'h = {h_max*1e6:.0f} мкм',
}

for tag, h_val in [('h_min', h_min), ('h_nom', h_nom), ('h_max', h_max)]:
    tf = plant_tf(h_val)
    w_out, H = signal.freqs(tf.num, tf.den, worN=w)
    mag_db = 20 * np.log10(np.abs(H))
    phase_deg = np.angle(H, deg=True)
    axes[0].semilogx(w_out, mag_db, color=colors[tag], label=labels_h[tag], linewidth=1.8)
    axes[1].semilogx(w_out, phase_deg, color=colors[tag], label=labels_h[tag], linewidth=1.8)

axes[0].set_ylabel('Амплитуда, дБ')
axes[0].set_title('АЧХ (ЛАЧХ)')
axes[0].legend()
axes[0].set_xlim([w[0], w[-1]])

axes[1].set_ylabel('Фаза, °')
axes[1].set_xlabel('Частота ω, рад/с')
axes[1].set_title('ФЧХ (ЛФЧХ)')
axes[1].legend()
axes[1].set_xlim([w[0], w[-1]])
axes[1].set_ylim([-270, 10])
axes[1].axhline(-180, color='k', linestyle='--', linewidth=0.8, alpha=0.5, label='-180°')

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'bode_plant.png'), bbox_inches='tight')
plt.close()
print("  Сохранено: bode_plant.png")

# ══════════════════════════════════════════════════════════════════════════════
# 2. СИНТЕЗ ПИД-РЕГУЛЯТОРА (частотный метод + оптимизация)
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 2. Синтез ПИД-регулятора ===")

# Используем h_max как расчётный (наихудший случай)
tf_plant_max = plant_tf(h_max)

# Полная разомкнутая система с датчиком
# W_ol = W_pid * W_plant * W_sens
# Замкнутая система синтезируется численно

def pid_tf_params(Kp, Ti, Td):
    """ПИД: Kp*(1 + 1/(Ti*p) + Td*p) = Kp*(Ti*Td*p² + Ti*p + 1) / (Ti*p)"""
    if Ti <= 0 or Td < 0:
        return None, None
    num = Kp * np.array([Ti * Td, Ti, 1.0])
    den = np.array([Ti, 0.0])
    return num, den

def closed_loop_step(Kp, Ti, Td, h=h_max, t_end=3.0, n_points=3000):
    """Вычислить переходную характеристику замкнутой системы с ПИД"""
    tf_p = plant_tf(h)
    # Числитель и знаменатель объекта
    Gnum = tf_p.num
    Gden = tf_p.den

    # ПИД числитель/знаменатель
    Cnum = Kp * np.array([Ti * Td, Ti, 1.0])
    Cden = np.array([Ti, 0.0])

    # Датчик числитель/знаменатель
    Snum = [Ku]
    Sden = [K3, 1.0]

    # W_ol = C * G * S  (числитель = произведение числителей, знаменатель = произведение знаменателей)
    ol_num = np.polymul(np.polymul(Cnum, Gnum), Snum)
    ol_den = np.polymul(np.polymul(Cden, Gden), Sden)

    # Замкнутая: W_cl = C*G / (1 + C*G*S)
    # числитель замкнутой = C*G*den_S (выход — N2, вход — задание)
    cl_num = np.polymul(np.polymul(Cnum, Gnum), Sden)  # без датчика в прямой цепи
    # Знаменатель замкнутой = Cden*Gden*Sden + Cnum*Gnum*Snum
    cl_den = np.polyadd(np.polymul(np.polymul(Cden, Gden), Sden),
                        np.polymul(np.polymul(Cnum, Gnum), Snum))

    try:
        sys_cl = signal.TransferFunction(cl_num, cl_den)
        t = np.linspace(0, t_end, n_points)
        t_out, y_out = signal.step(sys_cl, T=t)
        return t_out, y_out
    except Exception:
        return None, None


def quality_metrics(t, y, band=0.05):
    """Вычислить tp (время переходного процесса ±5%) и перерегулирование σ%
    По ГОСТ принято использовать полосу ±5% для систем автоматического регулирования"""
    if t is None or y is None or len(y) == 0:
        return 1e9, 1e9
    y_ss = y[-1]
    if abs(y_ss) < 1e-12:
        return 1e9, 1e9
    yn = y / y_ss
    sigma = (np.max(yn) - 1.0) * 100.0
    # Время вхождения в полосу ±5%
    in_band = np.abs(yn - 1.0) <= band
    # Найти последний момент выхода из полосы
    outside = np.where(~in_band)[0]
    if len(outside) == 0:
        tp = t[0]
    else:
        tp = t[outside[-1]]
    return tp, sigma


def cost_function(params):
    Kp, Ti, Td = params
    if Kp <= 0 or Ti <= 0 or Td < 0:
        return 1e9
    t, y = closed_loop_step(Kp, Ti, Td, h=h_max)
    if t is None:
        return 1e9
    tp, sigma = quality_metrics(t, y)
    # Целевая функция: минимизировать tp при σ < 0.6%
    # Строгий штраф за перерегулирование, мягкий за медленность
    J_tp    = max(0, tp - tp_req) * 200.0      # штраф за нарушение tp
    J_sigma = max(0, sigma - 0.6) * 8000.0     # очень жёсткий штраф за превышение σ
    J_speed = tp * 8.0                          # предпочитаем быстроту
    return J_tp + J_sigma + J_speed


# Начальная точка: грубые оценки
# ωc ≈ 4/tp_req ≈ 6.67 рад/с
# Kp_0 ≈ 1/K_max * ωc * T2  (упрощённая оценка)
Kp0 = 1.0 / K_max * 3.0
Ti0 = 5 * T1
Td0 = 0.5 * Tm

print(f"  Начальное приближение: Kp={Kp0:.4f}, Ti={Ti0:.4f}, Td={Td0:.6f}")

from scipy.optimize import differential_evolution

# Используем differential_evolution для глобальной оптимизации
bounds = [(0.1, 200.0), (0.05, 5.0), (0.001, 2.0)]

print("  Запуск глобальной оптимизации (differential_evolution)...")
result = differential_evolution(
    cost_function, bounds,
    seed=42, maxiter=500, tol=1e-6,
    popsize=20, mutation=(0.5, 1.5), recombination=0.9,
    workers=1
)

Kp_opt, Ti_opt, Td_opt = result.x
print(f"  Оптимальные параметры: Kp={Kp_opt:.4f}, Ti={Ti_opt:.4f} с, Td={Td_opt:.6f} с")

t_check, y_check = closed_loop_step(Kp_opt, Ti_opt, Td_opt, h=h_max)
tp_check, sigma_check = quality_metrics(t_check, y_check)
print(f"  Проверка (h_max): tp={tp_check:.3f} с, σ={sigma_check:.3f}%")

# ══════════════════════════════════════════════════════════════════════════════
# 3. ДИАГРАММЫ БОДЕ РАЗОМКНУТОЙ СИСТЕМЫ С РЕГУЛЯТОРОМ
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 3. Диаграммы Боде разомкнутой системы с регулятором ===")

def open_loop_tf_num_den(Kp, Ti, Td, h):
    """Числитель и знаменатель разомкнутой системы W_ol = C * G * S"""
    tf_p = plant_tf(h)
    Gnum = tf_p.num
    Gden = tf_p.den
    Cnum = Kp * np.array([Ti * Td, Ti, 1.0])
    Cden = np.array([Ti, 0.0])
    Snum = [Ku]
    Sden = [K3, 1.0]
    ol_num = np.polymul(np.polymul(Cnum, Gnum), Snum)
    ol_den = np.polymul(np.polymul(Cden, Gden), Sden)
    return ol_num, ol_den

fig, axes = plt.subplots(2, 1, figsize=(10, 7))
fig.suptitle('Диаграммы Боде разомкнутой системы с ПИД-регулятором\nВариант 5')

for tag, h_val in [('h_min', h_min), ('h_nom', h_nom), ('h_max', h_max)]:
    ol_num, ol_den = open_loop_tf_num_den(Kp_opt, Ti_opt, Td_opt, h_val)
    w_out, H = signal.freqs(ol_num, ol_den, worN=w)
    mag_db = 20 * np.log10(np.abs(H) + 1e-15)
    phase_deg = np.angle(H, deg=True)
    axes[0].semilogx(w_out, mag_db, color=colors[tag], label=labels_h[tag], linewidth=1.8)
    axes[1].semilogx(w_out, phase_deg, color=colors[tag], label=labels_h[tag], linewidth=1.8)

axes[0].axhline(0, color='k', linestyle='--', linewidth=0.8, alpha=0.7)
axes[0].set_ylabel('Амплитуда, дБ')
axes[0].set_title('ЛАЧХ разомкнутой системы с ПИД')
axes[0].legend()
axes[0].set_xlim([w[0], w[-1]])

axes[1].axhline(-180, color='k', linestyle='--', linewidth=0.8, alpha=0.7, label='-180°')
axes[1].set_ylabel('Фаза, °')
axes[1].set_xlabel('Частота ω, рад/с')
axes[1].set_title('ЛФЧХ разомкнутой системы с ПИД')
axes[1].legend()
axes[1].set_xlim([w[0], w[-1]])

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'bode_open_loop.png'), bbox_inches='tight')
plt.close()
print("  Сохранено: bode_open_loop.png")

# Запасы устойчивости (для h_max)
ol_num_max, ol_den_max = open_loop_tf_num_den(Kp_opt, Ti_opt, Td_opt, h_max)
try:
    ol_sys = signal.TransferFunction(ol_num_max, ol_den_max)
    w_out_max, H_max = signal.freqs(ol_num_max, ol_den_max, worN=np.logspace(-2, 4, 5000))
    mag_max = np.abs(H_max)
    phase_max = np.angle(H_max, deg=True)

    # Частота среза (0 дБ)
    idx_c = np.where(np.diff(np.sign(mag_max - 1.0)))[0]
    if len(idx_c) > 0:
        wc = w_out_max[idx_c[-1]]
        phase_at_wc = np.interp(wc, w_out_max, phase_max)
        PM = 180 + phase_at_wc
        print(f"  Частота среза ωc = {wc:.3f} рад/с, запас по фазе = {PM:.1f}°")
    else:
        print("  Не удалось найти частоту среза")

    # Частота фазового перехода (-180°)
    idx_pc = np.where(np.diff(np.sign(phase_max + 180.0)))[0]
    if len(idx_pc) > 0:
        wpc = w_out_max[idx_pc[0]]
        mag_at_wpc = np.interp(wpc, w_out_max, mag_max)
        GM = -20 * np.log10(mag_at_wpc + 1e-15)
        print(f"  Частота фазового перехода ωπ = {wpc:.3f} рад/с, запас по амплитуде = {GM:.1f} дБ")
    else:
        print("  Не удалось найти частоту фазового перехода")
except Exception as e:
    print(f"  Ошибка при вычислении запасов: {e}")

# ══════════════════════════════════════════════════════════════════════════════
# 4. ПЕРЕХОДНАЯ ХАРАКТЕРИСТИКА ЗАМКНУТОЙ СИСТЕМЫ
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 4. Переходная характеристика замкнутой системы ===")

fig, axes = plt.subplots(1, 2, figsize=(13, 5))
fig.suptitle('Переходная характеристика замкнутой системы с ПИД-регулятором\nВариант 5')

for ax_idx, (ax, h_design) in enumerate(zip(axes, [h_max, h_nom])):
    h_label = f"h = {h_design*1e6:.0f} мкм"
    t, y = closed_loop_step(Kp_opt, Ti_opt, Td_opt, h=h_design, t_end=3.0)
    if t is not None:
        y_ss = y[-1]
        yn = y / y_ss if abs(y_ss) > 1e-12 else y
        tp_val, sigma_val = quality_metrics(t, y)

        ax.plot(t, yn, color='tab:blue', linewidth=2, label='Переходная характеристика')
        ax.axhline(1.0, color='gray', linestyle='--', linewidth=1, label='Установившееся значение')
        ax.axhline(1.02, color='green', linestyle=':', linewidth=1, alpha=0.7, label='±2% полоса')
        ax.axhline(0.98, color='green', linestyle=':', linewidth=1, alpha=0.7)
        ax.axvline(tp_val, color='red', linestyle='--', linewidth=1,
                   label=f'tп = {tp_val:.3f} с')

        ax.set_xlabel('Время, с')
        ax.set_ylabel('h(t) / h(∞)')
        ax.set_title(f'{h_label}\ntп = {tp_val:.3f} с,  σ = {sigma_val:.3f}%')
        ax.legend(fontsize=9)
        ax.set_xlim([0, 3.0])
        ax.set_ylim([-0.05, 1.3])
        print(f"  {h_label}: tп = {tp_val:.3f} с, σ = {sigma_val:.3f}%")

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'step_response.png'), bbox_inches='tight')
plt.close()
print("  Сохранено: step_response.png")

# ══════════════════════════════════════════════════════════════════════════════
# 5. ИССЛЕДОВАНИЕ РОБАСТНОСТИ (h_min, h_nom, h_max)
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 5. Исследование робастности ===")

fig, ax = plt.subplots(figsize=(10, 6))
fig.suptitle('Переходные характеристики при различных значениях припуска h\n(Анализ робастности системы)')

h_vals  = [h_min, h_nom, h_max]
h_tags  = ['h_min', 'h_nom', 'h_max']
h_cols  = ['tab:blue', 'tab:green', 'tab:red']
h_labs  = [f'h_min = {h_min*1e6:.0f} мкм', f'h_nom = {h_nom*1e6:.0f} мкм', f'h_max = {h_max*1e6:.0f} мкм']

results = {}
for h_val, col, lab in zip(h_vals, h_cols, h_labs):
    t, y = closed_loop_step(Kp_opt, Ti_opt, Td_opt, h=h_val, t_end=3.0)
    if t is not None:
        y_ss = y[-1]
        yn = y / y_ss if abs(y_ss) > 1e-12 else y
        tp_val, sigma_val = quality_metrics(t, y)
        results[lab] = (tp_val, sigma_val)
        ax.plot(t, yn, color=col, linewidth=2, label=f'{lab}  (tп={tp_val:.2f}с, σ={sigma_val:.3f}%)')
        print(f"  {lab}: tп = {tp_val:.3f} с, σ = {sigma_val:.3f}%")

ax.axhline(1.0, color='gray', linestyle='--', linewidth=1)
ax.axhline(1.02, color='green', linestyle=':', linewidth=1, alpha=0.7)
ax.axhline(0.98, color='green', linestyle=':', linewidth=1, alpha=0.7)
ax.axvline(tp_req, color='orange', linestyle='-.', linewidth=1.5, label=f'Требование tп = {tp_req} с')
ax.set_xlabel('Время, с')
ax.set_ylabel('h(t) / h(∞)  (нормированный выход)')
ax.set_title('Переходные характеристики при различных h')
ax.legend()
ax.set_xlim([0, 3.0])
ax.set_ylim([-0.05, 1.5])

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'robustness.png'), bbox_inches='tight')
plt.close()
print("  Сохранено: robustness.png")

# ══════════════════════════════════════════════════════════════════════════════
# 6. ЧАСТОТНАЯ ХАРАКТЕРИСТИКА ПИД-РЕГУЛЯТОРА
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 6. АЧХ/ФЧХ ПИД-регулятора ===")

# W_pid(jw) = Kp * (1 + 1/(Ti*jw) + Td*jw)
w_pid = np.logspace(-2, 3, 2000)
H_pid = Kp_opt * (1 + 1 / (1j * w_pid * Ti_opt) + 1j * w_pid * Td_opt)

fig, axes = plt.subplots(2, 1, figsize=(10, 6))
fig.suptitle('Амплитудно-фазовая характеристика ПИД-регулятора')

axes[0].semilogx(w_pid, 20 * np.log10(np.abs(H_pid)), color='tab:purple', linewidth=2)
axes[0].set_ylabel('Амплитуда, дБ')
axes[0].set_title('ЛАЧХ ПИД-регулятора')
axes[0].set_xlim([w_pid[0], w_pid[-1]])

axes[1].semilogx(w_pid, np.angle(H_pid, deg=True), color='tab:orange', linewidth=2)
axes[1].set_ylabel('Фаза, °')
axes[1].set_xlabel('Частота ω, рад/с')
axes[1].set_title('ЛФЧХ ПИД-регулятора')
axes[1].set_xlim([w_pid[0], w_pid[-1]])

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'pid_bode.png'), bbox_inches='tight')
plt.close()
print("  Сохранено: pid_bode.png")

# ══════════════════════════════════════════════════════════════════════════════
# 7. СТРУКТУРНАЯ СХЕМА (текстовая схема через matplotlib)
# ══════════════════════════════════════════════════════════════════════════════
print("\n=== 7. Структурная схема ===")

fig, ax = plt.subplots(figsize=(13, 5))
ax.set_xlim(0, 14)
ax.set_ylim(0, 6)
ax.axis('off')
ax.set_facecolor('white')
fig.patch.set_facecolor('white')

def box(ax, x, y, w, h, text, fontsize=9, color='lightblue'):
    from matplotlib.patches import FancyBboxPatch
    rect = FancyBboxPatch((x - w/2, y - h/2), w, h,
                           boxstyle="round,pad=0.1", fc=color, ec='navy', lw=1.5)
    ax.add_patch(rect)
    ax.text(x, y, text, ha='center', va='center', fontsize=fontsize, wrap=True,
            multialignment='center')

def arrow(ax, x1, y1, x2, y2):
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(arrowstyle='->', color='black', lw=1.5))

def line(ax, x1, y1, x2, y2):
    ax.plot([x1, x2], [y1, y2], 'k-', lw=1.5)

# Setpoint
ax.text(0.3, 3.0, 'N₂*\n(задание)', ha='center', va='center', fontsize=10)
arrow(ax, 0.8, 3.0, 1.4, 3.0)

# Summing junction
circle = plt.Circle((1.7, 3.0), 0.3, color='white', ec='black', lw=1.5)
ax.add_patch(circle)
ax.text(1.7, 3.0, '∑', ha='center', va='center', fontsize=14)
ax.text(1.5, 3.25, '+', fontsize=10, color='green')
ax.text(1.5, 2.65, '−', fontsize=10, color='red')

arrow(ax, 2.0, 3.0, 2.5, 3.0)
ax.text(2.25, 3.2, 'e(t)', fontsize=9)

# PID
box(ax, 3.2, 3.0, 1.2, 0.8, 'ПИД\nрегулятор', color='#AED6F1')
arrow(ax, 3.8, 3.0, 4.4, 3.0)
ax.text(4.1, 3.2, 'U(t)', fontsize=9)

# Motor
box(ax, 5.2, 3.0, 1.2, 0.8, 'Двигатель\nпостоянного\nтока', fontsize=8, color='#A9DFBF')
arrow(ax, 5.8, 3.0, 6.4, 3.0)
ax.text(6.1, 3.2, 'ω(t)', fontsize=9)

# Feed mechanism
box(ax, 7.2, 3.0, 1.2, 0.8, 'Механизм\nподачи\nS=k₁·ω', fontsize=8, color='#FAD7A0')
arrow(ax, 7.8, 3.0, 8.4, 3.0)

# Cutting power
box(ax, 9.3, 3.0, 1.4, 0.8, 'Мощность\nрезания\nN₁=k₂·h·S', fontsize=8, color='#F1948A')
arrow(ax, 10.0, 3.0, 10.6, 3.0)

# Wheel drive
box(ax, 11.5, 3.0, 1.4, 0.8, 'Привод\nшлифкруга\nT₂p+1', fontsize=8, color='#D2B4DE')
arrow(ax, 12.2, 3.0, 12.8, 3.0)

ax.text(13.2, 3.0, 'N₂(t)', ha='center', va='center', fontsize=10, fontweight='bold')

# Feedback line
line(ax, 13.0, 3.0, 13.0, 1.5)
line(ax, 13.0, 1.5, 1.7, 1.5)
arrow(ax, 1.7, 1.5, 1.7, 2.7)

# Sensor in feedback
box(ax, 7.35, 1.5, 1.3, 0.7, 'Датчик\nKᵤ/(K₃p+1)', fontsize=8, color='#D5D8DC')
line(ax, 8.0, 1.5, 13.0, 1.5)
arrow(ax, 6.7, 1.5, 1.7, 1.5)

ax.text(7.35, 1.0, 'Цепь обратной связи', ha='center', fontsize=9, color='gray', style='italic')

ax.set_title('Структурная схема системы стабилизации мощности резания\n(Вариант 5)', fontsize=12)
plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, 'block_diagram.png'), bbox_inches='tight', dpi=150)
plt.close()
print("  Сохранено: block_diagram.png")

# ══════════════════════════════════════════════════════════════════════════════
# 8. ИТОГОВАЯ СВОДКА
# ══════════════════════════════════════════════════════════════════════════════
print("\n" + "="*60)
print("ИТОГОВЫЕ ПАРАМЕТРЫ ПИД-РЕГУЛЯТОРА")
print("="*60)
print(f"  Kp = {Kp_opt:.4f}")
print(f"  Ti = {Ti_opt:.4f} с")
print(f"  Td = {Td_opt:.6f} с")
print(f"  Ki = Kp/Ti = {Kp_opt/Ti_opt:.4f}")
print(f"  Kd = Kp*Td = {Kp_opt*Td_opt:.6f}")
print()
print("ПРОВЕРКА КАЧЕСТВА")
print("-"*60)

all_ok = True
for h_val, lab in zip(h_vals, h_labs):
    t, y = closed_loop_step(Kp_opt, Ti_opt, Td_opt, h=h_val)
    tp_v, sigma_v = quality_metrics(t, y)
    tp_ok = "OK" if tp_v <= tp_req else "НАРУШЕНО"
    sig_ok = "OK" if sigma_v <= 0.6 else "НАРУШЕНО"
    if tp_ok != "OK" or sig_ok != "OK":
        all_ok = False
    print(f"  {lab:35s}: tп={tp_v:.3f}с [{tp_ok}], σ={sigma_v:.3f}% [{sig_ok}]")

print()
if all_ok:
    print("  ✓ Все требования выполнены!")
else:
    print("  ✗ Некоторые требования нарушены")

print("\nПлоты сохранены в:", PLOTS_DIR)
print("  - bode_plant.png")
print("  - bode_open_loop.png")
print("  - step_response.png")
print("  - robustness.png")
print("  - pid_bode.png")
print("  - block_diagram.png")

# Сохраним параметры в файл для использования в отчёте
import json
params = {
    'Kp': float(Kp_opt),
    'Ti': float(Ti_opt),
    'Td': float(Td_opt),
    'Tm': float(Tm),
    'K_max': float(K_max),
    'K_nom': float(K_nom),
    'K_min': float(K_min),
}
for h_val, lab in zip(h_vals, h_labs):
    t, y = closed_loop_step(Kp_opt, Ti_opt, Td_opt, h=h_val)
    tp_v, sigma_v = quality_metrics(t, y)
    key = lab.split('=')[0].strip().replace(' ', '_')
    params[f'tp_{key}'] = float(tp_v)
    params[f'sigma_{key}'] = float(sigma_v)

with open(r"D:\school\ТАУ\Курсовая\sim_results.json", 'w', encoding='utf-8') as f:
    json.dump(params, f, ensure_ascii=False, indent=2)
print("\nПараметры сохранены: sim_results.json")
