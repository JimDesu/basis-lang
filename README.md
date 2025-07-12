# The Basis Programming Language.  

This is a part-time side project done at the pace of a parent's spare time; it will be a while before there's anything usable here.

## Informal Semantics
Given program state as a tuple $\langle V,\Phi,\Sigma \rangle$ where
* V is the current verb to be executed:
    * $\overrightarrow{v}$ represents the continuation from $v$
    * $exec(c)$ executes a user defined command
    * $fail(\phi)$ sets a failure state
    * $recover$ recovers from a failure status
    * $recover(\phi,\sigma,c)$ recovers a particular failure state type, binding the failure to $\sigma$ in $exec(c)$
    * $scope(c)$ executes at the current scope boundary
    * $scopefail(c)$ executes c at the current scope boundary under a failure
    * $rewind(v)$ continues execution at a previous verb in the current scope
* $\Phi$ represents the current failure status:
    * $\epsilon$ represents no current failure
    * $\phi$ represents a particular failure
* $\Sigma$ represents the current variable state
    * $\sigma/c$ is sigma bound within the scope of c  

General excution is described by the following rules:

$$
\begin{align}
\text{normal execution}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies & \langle \vec{v}, \epsilon, \Sigma' \rangle \\
\text{generating failure}\quad & v = fail(\phi) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{command failure}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{skips from failure}\quad & v \in \{exec(c),rewind(w),fail(\gamma)\} \quad \langle v, \phi, \Sigma \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{generic recovery}\quad & v=recover \quad \langle v, \phi, \Sigma \rangle & \implies & \langle \vec{v}, \epsilon, \Sigma \rangle \\
\text{specific recovery}\quad & v=recover(\phi,\sigma,c) \quad \langle v, \phi, \Sigma \rangle & \implies & \langle c, \epsilon, \Sigma+\sigma/c \rangle \quad \vec{c} \leftarrow \vec{v} \\
\text{recovery failure}\quad & v=recover(\alpha,\sigma,c) \quad \langle v, \phi, \Sigma \rangle \quad \phi \neq \sigma & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{scope boundary}\quad & v=scope(c) \quad \kappa\in\Phi \quad \langle v, \kappa, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle \quad \vec{c} \leftarrow \langle \vec{v},\kappa,\Sigma' \rangle \\
\text{scope boundary under failure}\quad & v=scopefail(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle \vec{v},\epsilon,\Sigma \rangle \\
\text{} & v=scopefail(c) \quad \langle v, \phi, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle \quad \vec{c} \leftarrow \langle \vec{v},\phi,\Sigma' \rangle \\
\text{looping}\quad & v=rewind(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle
\end{align}
$$

