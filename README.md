# The Basis programming language.  

This is currently in the ideation stage; there's nothing here worth looking at.

Given state as a tuple $\langle C,\Phi,\Sigma \rangle$ where
* V represnets the current command to be performed:
    * $exec(c)$ executes a user defined command
    * $recover$ recovers from a failure status
    * $recover(\phi,\sigma)$ recovers a particular failure state type, binding the failure to $\sigma$
    * $fail(\phi)$ sets a failure state
    * $next(v)$ is the next verb to be executed after $v$
* $\Phi$ represents the current failure status:
    * $\epsilon$ represents no current failure
    * $\phi$ represents a particular failure
* $\Sigma$ represents the current variable state.  

Executing a fail verb advances to the next verb, with the failure state set.  

$$
\Gamma v = fail(\phi) \quad \langle v, \epsilon, \Sigma \rangle \implies \langle next(v), \phi, \Sigma \rangle 
$$

General excution rules:

$$
\begin{align}
\text{normal execution}\quad & \langle v, \epsilon, \Sigma \rangle & \Downarrow & \langle v, \epsilon, \Sigma' \rangle & \implies \langle next(v), \epsilon, \Sigma' \rangle \\
\text{failure retains prior state}\quad & \langle v, \epsilon, \Sigma \rangle & \Downarrow & \langle c, \phi, \Sigma' \rangle & \implies \langle next(v), \phi, \Sigma \rangle 
\end{align}
$$

