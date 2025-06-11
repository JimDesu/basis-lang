# The Basis programming language.  

This is currently in the ideation stage; there's nothing here worth looking at.

## Informal Semantics
Given state as a tuple $\langle C,\Phi,\Sigma \rangle$ where
* V represnets the current verb to be executed:
    * $\overrightarrow{v}$ represents the continuation from $v$
    * $exec(c)$ executes a user defined command
    * $recover$ recovers from a failure status
    * $recover(\phi,\sigma,c)$ recovers a particular failure state type, binding the failure to $\sigma$ in $exec(c)$
    * $fail(\phi)$ sets a failure state
* $\Phi$ represents the current failure status:
    * $\epsilon$ represents no current failure
    * $\phi$ represents a particular failure
* $\Sigma$ represents the current variable state
    * $\sigma/c$ is sigma bound within the scope of c  

General excution rules:

$$
\begin{align}
\text{generating failure}\quad & \Gamma v = fail(\phi) \quad \langle v, \epsilon, \Sigma \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle \\
\text{normal execution}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies \langle \vec{v}, \epsilon, \Sigma' \rangle \\
\text{command failure}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle \\
\text{failure skips most verbs}\quad & \Gamma v=exec(c) \quad \langle v, \phi, \Sigma \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle \\
\text{generic recovery}\quad & \Gamma v=recover \quad \langle v, \phi, \Sigma \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle 
\end{align}
$$

