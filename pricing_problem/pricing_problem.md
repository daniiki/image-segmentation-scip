---
header-includes:
    - \newcommand{\superpixels}{\mathcal{S}}
    - \DeclareMathOperator*{\argmin}{arg\,min}
    - \DeclareMathOperator*{\Out}{Out}
    - \DeclareMathOperator*{\In}{In}
---

given:

- $\superpixels$: set of superpixels
- $\mathcal{P}$: set of partitions
- $y_s\geq0, \forall s\in\superpixels$: color of superpixel 
- $k$: number of partitions to select
- $\gamma_P=\sum_{s\in P}|c_P-y_s|$:
  fitting error that we make when selecting $P$,
  where $c_P$ is the fitting value,
  \begin{equation}
    c_P=\argmin_{c\geq0}\sum_{s\in P}|c-y_s|
  \end{equation}

variables:

- $x_P\in\{0,1\}, \forall P\in\mathcal{P}$: $P$ is selected

master problem:
\begin{align}
    & \min & \sum_{p\in\mathcal{P}} \gamma_P\cdot x_P \\
    & \text{s.t.} & \sum_{\{P\in\mathcal{P}:s\in P\}} x_P &= 1 & \forall s\in\superpixels \\
    && \sum_{P\in\mathcal{P}} x_P &= k \\
    && x_P &\in [0,1]
\end{align}

dual variables:

- $\mu_s, \forall s\in\superpixels$: corresponding to (3)
- $\lambda$: corresponding to (4)

dual problem:
\begin{align}
    & \max & \sum_{s\in\superpixels} \mu_s + k\cdot\lambda \\
    & \text{s.t.} & \sum_{s\in P} \mu_s + \lambda &\leq \gamma_P & \forall P\in\mathcal{P} \\
    && \mu_s & \text{ free} \\
    && \lambda & \text{ free}
\end{align}

pricing problem:
\begin{align}
    & \min & -\sum_{s\in\superpixels} x_s\cdot\mu_s + \sum_{s\in\superpixels} x_s\cdot|c_P-y_s| \\
    & \text{s.t.} & \text{connectivity constraints} \\
    && x_s & \in\{0,1\} \\
    && c_P & \text{ free}
\end{align}
becomes:
\begin{align}
    & \min & -\sum_{s\in\superpixels} x_s\cdot\mu_s + \sum_{s\in\superpixels} \delta_s \\
    & \text{s.t.} & c_P-y_s-M(1-x_s) &\leq \delta_s &\forall s \in \superpixels \\
    && y_s-c_P-M(1-x_s) &\leq \delta_s &\forall s \in \superpixels \\
    && \delta_s &\leq M\cdot x_s &\forall s \in \superpixels \\
    && \text{connectivity constraints} \\
    && x_s & \in\{0,1\} \\
    && c_P & \text{ free} \\
    && \delta_s &\geq0
\end{align}
where (17) is optional
and $M$ can be chosen as 
\begin{equation}
    M = \max_{s \in \superpixels} y_s
\end{equation}

connectivity constraints: 
\begin{align}
    \sum_{vw \in \Out(v)}e_{vw}^T - \sum_{wv \in \In(v)}e_{wv}^T &=
    \begin{cases}
        \sum_{w \in \superpixels}x_w^T-1 & \text{if }v=T \\
        -x_v^T & \text{otherwise}
    \end{cases}
    & \forall v \in \superpixels \\
    e_{vw}^T + e_{wv}^T &\leq (n-k)x_v^T &\forall (v,w) \in A \\
    e_{vw}^T + e_{wv}^T &\leq (n-k)x_w^T &\forall (v,w) \in A
\end{align}
