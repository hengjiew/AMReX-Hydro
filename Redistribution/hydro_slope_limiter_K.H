/**
 * \file hydro_slope_limiter_K.H
 * \addtogroup Redistribution
 * @{
 *
 */

using namespace amrex;

namespace {
  constexpr Real epsilon = 1.e-12;
}

AMREX_GPU_DEVICE AMREX_FORCE_INLINE
amrex::Real
amrex_calc_alpha_stencil(Real q_hat, Real q_max, Real q_min, Real state) noexcept
{
    const Real small = epsilon*std::max(amrex::Math::abs(q_max),amrex::Math::abs(q_min));
    Real alpha;

    if ((q_hat-state) > small) {
        alpha = std::min(1.0,(q_max-state)/(q_hat-state));
    } else if ((q_hat-state) < -small) {
        alpha = std::min(1.0,(q_min-state)/(q_hat-state));
    } else {
        alpha = 1.0;
    }
    return alpha;
}

AMREX_GPU_DEVICE AMREX_FORCE_INLINE
amrex::GpuArray<amrex::Real,AMREX_SPACEDIM>
amrex_calc_centroid_limiter(int i, int j, int k, int n,
                            amrex::Array4<amrex::Real const> const& state,
                            amrex::Array4<amrex::EBCellFlag const> const& flag,
                            const amrex::GpuArray<amrex::Real,AMREX_SPACEDIM>& slopes,
                            amrex::Array4<amrex::Real const> const& ccent) noexcept
{
    AMREX_D_TERM(amrex::Real xalpha = 1.0;,
                 amrex::Real yalpha = 1.0;,
                 amrex::Real zalpha = 1.0;);

    // Compute the limiters needed to keep the predicted q_hat between the max and min
#if (AMREX_SPACEDIM == 2)
    int kk = 0;
#elif (AMREX_SPACEDIM == 3)
    for(int kk(-1); kk<=1; kk++)
#endif
    {
     for(int jj(-1); jj<=1; jj++){
      for(int ii(-1); ii<=1; ii++){
        Real alpha = std::max(xalpha,yalpha);
#if (AMREX_SPACEDIM == 3)
        alpha = std::max(alpha,zalpha);
#endif
        if (flag(i,j,k).isConnected(ii,jj,kk) && alpha > 0.0)
        {
            AMREX_D_TERM(Real delta_x = ccent(i+ii,j+jj,k+kk,0) -  ccent(i,j,k,0) + static_cast<Real>(ii);,
                         Real delta_y = ccent(i+ii,j+jj,k+kk,1) -  ccent(i,j,k,1) + static_cast<Real>(jj);,
                         Real delta_z = ccent(i+ii,j+jj,k+kk,2) -  ccent(i,j,k,2) + static_cast<Real>(kk););

            Real q_hat = state(i,j,k,n) + AMREX_D_TERM(  delta_x * slopes[0],
                                                       + delta_y * slopes[1],
                                                       + delta_z * slopes[2]);

            Real q_max = std::max(state(i+ii,j+jj,k+kk,n),state(i,j,k,n));
            Real q_min = std::min(state(i+ii,j+jj,k+kk,n),state(i,j,k,n));

            if ( q_hat-q_max > amrex::Math::abs(epsilon*q_max) || q_hat-q_min < -1.0*amrex::Math::abs(epsilon*q_min) )
            {
                Real new_lim = amrex_calc_alpha_stencil(q_hat, q_max, q_min, state(i,j,k,n));

                if (amrex::Math::abs(delta_x) > epsilon) xalpha = std::min(xalpha,new_lim);
                if (amrex::Math::abs(delta_y) > epsilon) yalpha = std::min(yalpha,new_lim);
#if (AMREX_SPACEDIM == 3)
                if (amrex::Math::abs(delta_z) > epsilon) zalpha = std::min(zalpha,new_lim);
#endif
            }
        }
      }
     }
    }

    return {AMREX_D_DECL(xalpha,yalpha,zalpha)};
}
/** @} */
