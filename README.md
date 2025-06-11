# The Basis programming language.  

This is currently in the ideation stage; there's nothing here worth looking at.

## Informal Semantics
Given state as a tuple $\langle C,\Phi,\Sigma \rangle$ where
* V represnets the current command to be performed:
    * $\overrightarrow{v}$ represents the continuation from $v$
    * $exec(c)$ executes a user defined command
    * $recover$ recovers from a failure status
    * $recover(\phi,c)$ recovers a particular failure state type, binding the failure to $\sigma$ in $exec(c)$
    * $fail(\phi)$ sets a failure state
* $\Phi$ represents the current failure status:
    * $\epsilon$ represents no current failure
    * $\phi$ represents a particular failure
* $\Sigma$ represents the current variable state
    * $\sigma/c$ is sigma bound within the scope of c  

Executing a fail verb advances to the next verb, with the failure state set.  

$$
\Gamma v = fail(\phi) \quad \langle v, \epsilon, \Sigma \rangle \implies \langle \vec{v}, \phi, \Sigma \rangle 
$$

General excution rules:

$$
\begin{align}
\text{normal execution}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies \langle \vec{v}, \epsilon, \Sigma' \rangle \\
\text{failure retains prior state}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle \\
\text{failure skips most verbs}\quad & \Gamma v=exec(c) \quad \langle v, \phi, \Sigma \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle \\
\text{generic recovery}\quad & \Gamma v=recover \quad \langle v, \phi, \Sigma \rangle & \implies \langle \vec{v}, \phi, \Sigma \rangle 
\end{align}
$$

