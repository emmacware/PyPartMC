/*##################################################################################################
# This file is a part of PyPartMC licensed under the GNU General Public License v3 (LICENSE file)  #
# Copyright (C) 2022 University of Illinois Urbana-Champaign                                       #
# Authors: https://github.com/open-atmos/PyPartMC/graphs/contributors                              #
##################################################################################################*/

#pragma once

#include "pmc_resource.hpp"
#include "pybind11/stl.h"
#include "aero_data.hpp"
#include "bin_grid.hpp"

extern "C" void f_aero_mode_ctor(
    void *ptr
) noexcept;

extern "C" void f_aero_mode_dtor(
    void *ptr
) noexcept;

extern "C" void f_aero_mode_get_num_conc(
    const void *ptr,
    double *val
) noexcept;

extern "C" void f_aero_mode_set_num_conc(
    void *ptr,
    const double *val
) noexcept;

extern "C" void f_aero_mode_num_conc(
    const void *ptr,
    const void *bin_grid_ptr,
    const void *aero_data_ptr_c,
    void *arr_data,
    const int *arr_size
) noexcept;

extern "C" void f_aero_mode_get_n_spec(
    const void *ptr,
    int *len
) noexcept;

extern "C" void f_aero_mode_get_vol_frac(
    const void *ptr,
    void *arr_data,
    const int *arr_size
) noexcept;

extern "C" void f_aero_mode_set_vol_frac(
    void *ptr,
    const void *arr_data,
    const int *arr_size
) noexcept;

extern "C" void f_aero_mode_get_vol_frac_std(
    const void *ptr,
    void *arr_data,
    const int *arr_size
) noexcept;

extern "C" void f_aero_mode_set_vol_frac_std(
    void *ptr,
    const void *arr_data,
    const int *arr_size
) noexcept;

extern "C" void f_aero_mode_get_char_radius(
    const void *ptr,
    double *val
) noexcept;

extern "C" void f_aero_mode_set_char_radius(
    void *ptr,
    const double *val 
) noexcept;

extern "C" void f_aero_mode_get_gsd(
    const void *ptr,
    double *val
) noexcept;

extern "C" void f_aero_mode_set_gsd(
    void *ptr,
    const double *val
) noexcept;

extern "C" void f_aero_mode_set_type(
    void *ptr,
    const int *val
) noexcept;

extern "C" void f_aero_mode_get_type(
    const void *ptr,
    int *val
) noexcept;

extern "C" void f_aero_mode_set_name(
    void *ptr,
    const char *name_data, const int *name_size
) noexcept;

extern "C" void f_aero_mode_get_name(
    const void *ptr,
    char **name_data, int *name_size
) noexcept;

extern "C" void f_aero_mode_set_sampled(
    void *ptr,
    const void *rad_data,
    const void *num_conc_data,
    const int *arr_size
) noexcept; 

extern "C" void f_aero_mode_from_json(
    void *ptr,
    void *aero_data_ptr
) noexcept;

extern "C" void f_aero_mode_get_sample_num_conc(
    const void *ptr,
    void *sample_num_conc_data,
    const int *sample_num_conc_data_size
) noexcept;

extern "C" void f_aero_mode_get_sample_radius(
    const void *ptr,
    void *sample_radius_data,
    const int *sample_radius_data_size
) noexcept;

extern "C" void f_aero_mode_get_sample_bins(const void *ptr, int *n_bins) noexcept;

struct AeroMode {
    PMCResource ptr;

    AeroMode() :
        ptr(f_aero_mode_ctor, f_aero_mode_dtor)
    {}

    AeroMode(AeroData &aero_data, const nlohmann::json &json) :
        ptr(f_aero_mode_ctor, f_aero_mode_dtor)
    {
        if (json.size() != 1 || !json.is_object() || !json.begin().value().is_object())
            throw std::runtime_error("Single-element dict expected with mode name as key and mode params dict as value");
        check_mode_json(json.begin().value());
        JSONResourceGuard<InputJSONResource> guard(json, "", "mode_name");
        f_aero_mode_from_json(ptr.f_arg_non_const(), aero_data.ptr.f_arg_non_const());
        guard.check_parameters();
    }

    static void check_mode_json(const nlohmann::json &mode) {
        for (auto key : std::set<std::string>({"mass_frac", "mode_type"})) // TODO #320: more...
            if (mode.find(key) == mode.end())
                throw std::runtime_error("mode parameters dict must include key '" + key + "'");
        auto mass_frac = mode["mass_frac"];
            
        if (!mass_frac.is_array()) // TODO #320: check if all are single-element dicts
            throw std::runtime_error("mass_frac value must be a list of single-element dicts");
        if (!InputJSONResource::unique_keys(mass_frac))
            throw std::runtime_error("mass_frac keys must be unique");
        if (mode["mode_type"] == "sampled") {
            if (mode.find("size_dist") == mode.end())
                throw std::runtime_error("size_dist key must be set for mode_type=sampled");
            auto sd = mode["size_dist"];
            if (
                sd.size() != 2 || 
                !sd[0].is_object() || 
                sd[0].size() != 1 || 
                sd[1].size() != 1 ||
                sd[0].find("diam") == sd[0].end() ||
                sd[1].find("num_conc") == sd[1].end()
            )
                throw std::runtime_error("size_dist value must be an iterable of two single-element dicts (first with 'diam', second with 'num_conc' as keys)");
            auto diam = *sd[0].find("diam");
            auto num_conc = *sd[1].find("num_conc");
            if (diam.size() != num_conc.size() + 1)
                throw std::runtime_error("size_dist['num_conc'] must have len(size_dist['diam'])-1 elements");
        }
    }

    static auto get_num_conc(const AeroMode &self){
       double val;
       f_aero_mode_get_num_conc(self.ptr.f_arg(), &val);
       return val;
    }

    static void set_num_conc(AeroMode &self, const double &val){
       f_aero_mode_set_num_conc(self.ptr.f_arg_non_const(), &val);
    }

    static auto num_dist(const AeroMode &self, 
       const BinGrid &bin_grid, const AeroData &aero_data)
    {
       int len;
       f_bin_grid_size(bin_grid.ptr.f_arg(), &len);
       std::valarray<double> data(len);

       f_aero_mode_num_conc(
           self.ptr.f_arg(),
           bin_grid.ptr.f_arg(),
           aero_data.ptr.f_arg(),
           begin(data),
           &len
       );

       return data; 
    }

    static void set_vol_frac(AeroMode &self, const std::valarray<double>&data)
    {
        int len = data.size();
        int n_spec;
        f_aero_mode_get_n_spec(self.ptr.f_arg(), &n_spec);
        if (len!=n_spec)
            throw std::runtime_error("AeroData size mismatch");
        f_aero_mode_set_vol_frac(
            self.ptr.f_arg_non_const(),
            begin(data),
            &len
        );
    }

    static auto get_vol_frac(const AeroMode &self)
    {
       int len;
       f_aero_mode_get_n_spec(self.ptr.f_arg(), &len); 
       std::valarray<double> data(len);
       f_aero_mode_get_vol_frac(self.ptr.f_arg(), begin(data), &len);
       return data;
    }

    static void set_vol_frac_std(AeroMode &self, const std::valarray<double>&data)
    {
        int len = data.size();
        int n_spec;
        f_aero_mode_get_n_spec(self.ptr.f_arg(), &n_spec);
        if (len!=n_spec)
            throw std::runtime_error("AeroData size mismatch");
        f_aero_mode_set_vol_frac_std(
            self.ptr.f_arg_non_const(),
            begin(data),
            &len
        );
    }

    static auto get_vol_frac_std(const AeroMode &self)
    {
       int len;
       f_aero_mode_get_n_spec(self.ptr.f_arg(), &len);
       std::valarray<double> data(len);
       f_aero_mode_get_vol_frac_std(self.ptr.f_arg(), begin(data), &len);
       return data;
    }

    static auto get_char_radius(const AeroMode &self){
       double val;
       f_aero_mode_get_char_radius(self.ptr.f_arg(), &val);
       return val;
    }

    static void set_char_radius(AeroMode &self, const double &val){
       f_aero_mode_set_char_radius(self.ptr.f_arg_non_const(), &val);
    }

    static auto get_gsd(const AeroMode &self){
       double val;
       f_aero_mode_get_gsd(self.ptr.f_arg(), &val);
       return val;
    }

    static void set_gsd(AeroMode &self, const double &val) {
       f_aero_mode_set_gsd(self.ptr.f_arg_non_const(), &val);
    }

    static void set_sampled(AeroMode &self, const std::valarray<double>&sample_diams,
         const std::valarray<double>&sample_num_conc) {

        int len = sample_diams.size();
        {
            int len_num_conc = sample_num_conc.size();
            if (len != len_num_conc + 1)
                throw std::runtime_error("Diameter and number size mismatch");
        }

        f_aero_mode_set_sampled(
            self.ptr.f_arg_non_const(),
            begin(sample_diams),
            begin(sample_num_conc),
            &len
        );
    }

    static auto types() {
        static auto vec = std::vector<std::string>{
            "log_normal",
            "exp",
            "mono",
            "sampled"
        };
        return vec;
    };

    static void set_type(AeroMode &self, const std::string &mode_type) {
        auto found = false;
        auto type = 0;

        for (auto el : AeroMode::types()) {
            ++type;
            if (el == mode_type) {
                found = true;
                break;
            }
        }
        
        if (!found)
            throw std::invalid_argument("Invalid mode type.");

        f_aero_mode_set_type(self.ptr.f_arg_non_const(), &type);
    }

    static auto get_type(const AeroMode &self) {
        int type;
        f_aero_mode_get_type(self.ptr.f_arg(), &type);

        if (type <= 0 || (unsigned int)type > AeroMode::types().size())
            throw std::logic_error("Unknown mode type.");

        return AeroMode::types()[type - 1];
    }

    static void set_name(AeroMode &self, const std::string &name) {
        int size = name.size();
        f_aero_mode_set_name(self.ptr.f_arg_non_const(), name.c_str(), &size);
    }

    static auto get_name(const AeroMode &self) {
        std::string name;
        char* f_ptr = NULL;
        int size = 0;
        f_aero_mode_get_name(self.ptr.f_arg(), &f_ptr, &size);
        name.resize(size);
        for (int i = 0; i < size; ++i)
          name[i] = f_ptr[i];
        return name;
    }

    static auto get_sample_radius(const AeroMode &self) {
       int len;
       f_aero_mode_get_sample_bins(self.ptr.f_arg(), &len);
       len++;
       std::valarray<double> sample_radius(len);
       f_aero_mode_get_sample_radius(
          self.ptr.f_arg(),
          begin(sample_radius),
          &len
       );
       return sample_radius;
    }

    static auto get_sample_num_conc(const AeroMode &self) {
       int len;
       f_aero_mode_get_sample_bins(self.ptr.f_arg(), &len);
       std::valarray<double> sample_num_conc(len);
       f_aero_mode_get_sample_num_conc(
           self.ptr.f_arg(),
           begin(sample_num_conc),
           &len
       );
       return sample_num_conc;
    }
};
