#ifndef RTE_SW_H
#define RTE_SW_H

namespace rrtmgp_kernels
{
    extern "C" void apply_BC_0(
            int* ncol, int* nlay, int* ngpt,
            int* top_at_1, double* gpt_flux_dn);

    extern "C" void apply_BC_factor(
            int* ncol, int* nlay, int* ngpt,
            int* top_at_1, double* inc_flux,
            double* factor, double* flux_dn);

    /*
    extern "C" void lw_solver_noscat_GaussQuad(
            int* ncol, int* nlay, int* ngpt, int* top_at_1, int* n_quad_angs,
            double* gauss_Ds_subset, double* gauss_wts_subset,
            double* tau,
            double* lay_source, double* lev_source_inc, double* lev_source_dec,
            double* sfc_emis_gpt, double* sfc_source,
            double* gpt_flux_up, double* gpt_flux_dn);
            */

    // template<typename TF>
    // void apply_BC(
    //         int ncol, int nlay, int ngpt,
    //         int top_at_1, Array<TF,3>& gpt_flux_dn)
    // {
    //     apply_BC_0(
    //             &ncol, &nlay, &ngpt,
    //             &top_at_1, gpt_flux_dn.ptr());
    // }

    template<typename TF>
    void apply_BC(
            int ncol, int nlay, int ngpt, int top_at_1,
            const Array<TF,2>& inc_flux,
            const Array<TF,1>& factor,
            Array<TF,3>& gpt_flux)
    {
        apply_BC_factor(
                &ncol, &nlay, &ngpt,
                &top_at_1,
                const_cast<TF*>(inc_flux.ptr()),
                const_cast<TF*>(factor.ptr()),
                gpt_flux.ptr());
    }

    /*
    template<typename TF>
    void lw_solver_noscat_GaussQuad(
            int ncol, int nlay, int ngpt, int top_at_1, int n_quad_angs,
            const Array<TF,2>& gauss_Ds_subset,
            const Array<TF,2>& gauss_wts_subset,
            const Array<TF,3>& tau,
            const Array<TF,3>& lay_source,
            const Array<TF,3>& lev_source_inc, const Array<TF,3>& lev_source_dec,
            const Array<TF,2>& sfc_emis_gpt, const Array<TF,2>& sfc_source,
            Array<TF,3>& gpt_flux_up, Array<TF,3>& gpt_flux_dn)
    {
        lw_solver_noscat_GaussQuad(
                &ncol, &nlay, &ngpt, &top_at_1, &n_quad_angs,
                const_cast<TF*>(gauss_Ds_subset.ptr()),
                const_cast<TF*>(gauss_wts_subset.ptr()),
                const_cast<TF*>(tau.ptr()),
                const_cast<TF*>(lay_source.ptr()),
                const_cast<TF*>(lev_source_inc.ptr()),
                const_cast<TF*>(lev_source_dec.ptr()),
                const_cast<TF*>(sfc_emis_gpt.ptr()),
                const_cast<TF*>(sfc_source.ptr()),
                gpt_flux_up.ptr(),
                gpt_flux_dn.ptr());
    }
    */
}

template<typename TF>
class Rte_sw
{
    public:
        static void rte_sw(
                const std::unique_ptr<Optical_props_arry<TF>>& optical_props,
                const int top_at_1,
                const Array<TF,1>& mu0,
                const Array<TF,2>& inc_flux,
                const Array<TF,2>& sfc_alb_dir,
                const Array<TF,2>& sfc_alb_dif,
                std::unique_ptr<Fluxes_broadband<TF>>& fluxes)
        {
            const int ncol = optical_props->get_ncol();
            const int nlay = optical_props->get_nlay();
            const int ngpt = optical_props->get_ngpt();
            const int nband = optical_props->get_nband();

            Array<TF,3> gpt_flux_up ({ncol, nlay+1, ngpt});
            Array<TF,3> gpt_flux_dn ({ncol, nlay+1, ngpt});
            Array<TF,3> gpt_flux_dir({ncol, nlay+1, ngpt});

            Array<TF,2> sfc_alb_dir_gpt({ncol, ngpt});
            Array<TF,2> sfc_alb_dif_gpt({ncol, ngpt});

            expand_and_transpose(optical_props, sfc_alb_dir, sfc_alb_dir_gpt);
            expand_and_transpose(optical_props, sfc_alb_dif, sfc_alb_dif_gpt);

            // Upper boundary condition.
            // rrtmgp_kernels::apply_BC(ncol, nlay, ngpt, top_at_1, inc_flux, mu0, gpt_flux_dir);
            // rrtmgp_kernels::apply_BC(ncol, nlay, ngpt, top_at_1, gpt_flux_dn);

            /*
            // Run the radiative transfer solver
            rrtmgp_kernels::lw_solver_noscat_GaussQuad(
                    ncol, nlay, ngpt, top_at_1, n_quad_angs,
                    gauss_Ds_subset, gauss_wts_subset,
                    optical_props->get_tau(),
                    sources.get_lay_source(),
                    sources.get_lev_source_inc(), sources.get_lev_source_dec(),
                    sfc_emis_gpt, sources.get_sfc_source(),
                    gpt_flux_up, gpt_flux_dn);
                    */

            fluxes->reduce(gpt_flux_up, gpt_flux_dn, gpt_flux_dir, optical_props, top_at_1);
        }

        static void expand_and_transpose(
                const std::unique_ptr<Optical_props_arry<TF>>& ops,
                const Array<TF,2> arr_in,
                Array<TF,2>& arr_out)
        {
            const int ncol = arr_in.dim(2);
            const int nband = ops->get_nband();
            Array<int,2> limits = ops->get_band_lims_gpoint();

            for (int iband=1; iband<=nband; ++iband)
                for (int icol=1; icol<=ncol; ++icol)
                    for (int igpt=limits({1, iband}); igpt<=limits({2, iband}); ++igpt)
                        arr_out({icol, igpt}) = arr_in({iband, icol});
        }
};
#endif