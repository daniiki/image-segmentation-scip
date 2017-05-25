---
header-includes:
    - \newcommand{\superpixels}{\mathcal{S}}
---

given:

- $\superpixels$: set of superpixels
- $\mathcal{P}$: set of partitions
- $y_s, \forall s\in\superpixels$: color of superpixel 
- $k$: number of partitions to select
- $\gamma_P=\sum_{s\in P}|c_P-y_s|$:
  error that we make when selecting $P$,
  where $c_P$ is the average color of $P$,
  \begin{equation}
    c_P=\frac{1}{|P|}\sum_{s\in P}y_s
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
    & \text{s.t.} & \text{connectivity constraints}
\end{align}
with
\begin{equation}
    c_P = \frac{\sum_{s\in\superpixels}x_s\cdot y_s}{\sum_{s\in\superpixels}x_s}
\end{equation}
becomes:
\begin{align}
    & \min & -\sum_{s\in\superpixels} x_s\cdot y_s + \sum_{s\in\superpixels} x_s\cdot|c_P-y_s| \\
    & \text{s.t.} & \text{connectivity constraints} \\
    && ????
\end{align}
