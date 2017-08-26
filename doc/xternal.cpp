/** @file */

/** @mainpage Image segmentation using SCIP
 * 
 * \htmlonly <style>div.image img{width:300px;}</style> \endhtmlonly
 * 
 * This advanced practical dealt with image segmentation.
 * We used a mixed-interger programming framework called <a href="http://scip.zib.de/">SCIP</a>.
 * To read more about the classes and functions defined by SCIP,
 * please refer to its <a href="http://scip.zib.de/doc/html/">documentation</a>.
 * 
 * In our problem, we get an input image.
 * @image html input.png "Input image"
 * The pixels in the input image are clustered in so-called superpixels.
 * This reduces the number of variables and thereby makes the problem easier to solve.
 * The color of a superpixel is defined as the average color of its pixels.
 * @image html superpixels.png "Superpixels"
 * Finally, we get so-called master nodes as input.
 * These are special superpixels, in that every segment contains exactly one of them.
 * They are marked by \f$t_1,\dots,t_5\f$ in the following image.
 * Given this input, we want to cover the image with disjoint segments.
 * The superpixels in each segment should be similar in color.
 * They are marked by thick black lines in the image.
 * @image html segments.png "Master nodes and segments"
 * 
 * Mathematically, the input is the following:
 * - \f$\mathcal{S}\f$: set of superpixels
 * - \f$T\subseteq\mathcal{S}\f$: set of master nodes
 * - \f$\mathcal{P}\subset2^\mathcal{S}\f$: set of connected segments, which each contain extactly one master node
 * - \f$y_s\geq0\f$: color of superpixel \f$s\in\mathcal{S}\f$
 * - \f$k=|T|\f$: number of segments to cover the image with
 * - \f$r_P=\sum_{s\in P}|y_t-y_s|\f$: error of segment \f$P\in\mathcal{P}\f$,
 *   where $t\in T$ is the single master node for which \f$t\in P\f$
 * 
 * The master problem is
 * \f{align}{
 *   &\min\quad \sum_{P\in\mathcal{P}} r_P\cdot x_P\\
 *   &\text{s.t.}\quad \sum_{\{P\in\mathcal{P}:s\in P\}} x_P = 1 \quad\forall s\in\mathcal{S} \\
 *   &\phantom{\text{s.t.}\quad} \sum_{P\in\mathcal{P}} x_P = k \\
 *   &\phantom{\text{s.t.}\quad} x_P \in \{0,1\} \quad\forall P\in\mathcal{P}
 * \f}
 * It is implemented in main.cpp.
 * 
 * Since there are exponentially many segments, we use column generation.
 * Therefore, we need the dual problem, which has variables
 * - \f$\mu_s\f$ for all \f$s\in\mathcal{S}\f$: corresponding to the first constraint
 * - \f$\lambda\f$: corresponding to second one
 * The dual problem reads
 * \f{align}{
 *   &\max\quad \sum_{s\in\mathcal{S}} \mu_s + k\cdot\lambda \\
 *   &\text{s.t.}\quad \sum_{s\in P} \mu_s + \lambda \leq r_P \quad\forall P\in\mathcal{P} \\
 *   &\phantom{\text{s.t.}\quad} \mu_s \text{ free} \quad\forall s\in\mathcal{S} \\
 *   &\phantom{\text{s.t.}\quad} \lambda \text{ free}
 * \f}
 * 
 * There is one pricing problem per master node \f$t\f$.
 * It has a binary variable \f$x_s\f$ for every \f$s\in\mathcal{S}\f$ and reads
 * \f{align}{
 *   &\min\quad \underbrace{\sum_{s\in\mathcal{S}} x_s\cdot|y_t-y_s|}_{=r_{\{s\in\mathcal{S}\ :\ x_s=1\}}} - \sum_{s\in\mathcal{S}} x_s\cdot\mu_s \\
 *   &\text{s.t.}\quad \text{connectivity constraint} \\
 *   &\phantom{\text{s.t.}\quad} x_t = 1 \\
 *   &\phantom{\text{s.t.}\quad} x_{t'} = 0 \quad\forall t'\in T\setminus\{t\} \\
 *   &\phantom{\text{s.t.}\quad} x_s \in\{0,1\} \quad\forall s\in\mathcal{S}
 * \f}
 * The pricing problem, including a simple heuristic, is implemented in the class SegmentPricer.
 * 
 * To ensure connectivity of the new segment, we use a cutting planes approach.
 * We look at the subgraph with vertices \f$\{s\in\mathcal{S}:x_s=1\}\f$.
 * If a component \f$C\f$ is not connected to \f$t\f$, we add a cut for each \f$s\in C\f$:
 * \f[\sum_{s'\in\delta(C)}x_{s'} \geq x_s\f]
 * You can read more about this in the documentation for ConnectivityCons.
 */
