//
//  Copyright (C) 2010 - 2012 Creonic GmbH
//
//  This file is part of the Creonic simulation environment (CSE)
//  for communication systems.
//
/// \file
/// \brief  Hardware-compliant LDPC decoder module for the IEEE 802.11ad standard
/// \author Markus Fehrenz
/// \date   2012/01/24
//

#include "dec_ldpc_ieee_802_11ad.h"
#include "dec_ldpc_ieee_802_11ad_codes.hpp"

using namespace hlp_fct::logging;
using namespace cse_lib::ieee_802_11ad_codes;
using namespace std;

namespace cse_lib {

void Decoder_LDPC_IEEE_802_11ad::Set_LDPC_Parameters()
{

	/*
	 * Parameterize the _Share class with the parameters from the configuration.
	 */

	check_node_algorithm_ = dec_algorithm();
	num_lambda_min_       = num_lambda_min();
	esf_factor_           = esf_factor();
	bw_fract_             = bw_fract();
    num_partitions_       = num_partitions();
    threshold_            = threshold();

	// Calculate the maximum values that can be represented by the chosen quantization.
	max_msg_extr_         = (1 << (bw_extr() - 1)) - 1;  // max_msg_extr = 31
	max_msg_app_          = (1 << (bw_app()  - 1)) - 1;


	// Set LDPC code parameters.
	switch(ldpc_code())
	{

	case IEEE_802_11AD_P42_N672_R050:
		num_variable_nodes_ = 672;
		num_check_nodes_    = 336;
		src_parallelism_    = 42;
		dst_parallelism_    = 42;
		max_check_degree_   = 8;
		is_IRA_code_        = false;
		addr_vector_        = &ieee_802_11ad_p42_n672_r050_addr[0];
		shft_vector_        = &ieee_802_11ad_p42_n672_r050_shft[0];
		break;

	case IEEE_802_11AD_P42_N672_R062:
		num_variable_nodes_ = 672;
		num_check_nodes_    = 252;
		src_parallelism_    = 42;
		dst_parallelism_    = 42;
		max_check_degree_   = 10;
		is_IRA_code_        = false;
		addr_vector_        = &ieee_802_11ad_p42_n672_r062_addr[0];
		shft_vector_        = &ieee_802_11ad_p42_n672_r062_shft[0];
		break;

	case IEEE_802_11AD_P42_N672_R075:
		num_variable_nodes_ = 672;
		num_check_nodes_    = 168;
		src_parallelism_    = 42;
		dst_parallelism_    = 42;
		max_check_degree_   = 15;
		is_IRA_code_        = false;
		addr_vector_        = &ieee_802_11ad_p42_n672_r075_addr[0];
		shft_vector_        = &ieee_802_11ad_p42_n672_r075_shft[0];
		break;

	case IEEE_802_11AD_P42_N672_R081:
		num_variable_nodes_ = 672;
		num_check_nodes_    = 126;
		src_parallelism_    = 42;
		dst_parallelism_    = 42;
		max_check_degree_   = 16;
		is_IRA_code_        = false;
		addr_vector_        = &ieee_802_11ad_p42_n672_r081_addr[0];
		shft_vector_        = &ieee_802_11ad_p42_n672_r081_shft[0];
		break;

	}
}


void Decoder_LDPC_IEEE_802_11ad::Init()
{

	// Set code and decoder parameters
	Set_LDPC_Parameters();

	mean_iterations.Reset();
    flipped_bits.Reset();

	// Resize output buffers and internal RAMs.
	try
	{
		// Output RAM content for each iteration.
		output_bits().Resize(num_iterations(), num_variable_nodes_);
		output_bits_llr_app().Resize(num_iterations(), num_variable_nodes_);

		// Decoder RAMs
		app_ram_.Resize(dst_parallelism_, num_variable_nodes_ / dst_parallelism_);
		msg_ram_.Resize(dst_parallelism_, num_check_nodes_ * max_check_degree_ / dst_parallelism_);
	}
	catch(bad_alloc&)
	{
		Msg(ERROR, instance_name(), "Memory allocation failure!");
		throw;
	}

	param_list_.config_modified(false);
	input_data_list_.port_modified(false);
}


int Decoder_LDPC_IEEE_802_11ad::Run()
{

	unsigned int pchk_satisfied;
	unsigned int iter = 0;
	bool next_iter_is_last_iter = false; 
	bool last_iter = false; 

	decoding_successful().Write(false);
	num_modified_systematic_bits().Write(0);

	if(param_list_.config_modified())
		Init();

	// Read the channel values and store them in app_ram_.
	Init_APP_RAM(is_IRA_code_, input_bits_llr(), app_ram_);

	do
	{
		// Perform one ldpc decoder iteration.
		switch(scheduling())
		{
		case LAYERED:
			pchk_satisfied = Decode_Layered(app_ram_, msg_ram_, iter);
			break;

		case TWO_PHASE:
			pchk_satisfied = Decode_Two_Phase(app_ram_, msg_ram_, iter, app_parity_check());
			break;

		default:
			pchk_satisfied = 0;
			Msg(ERROR, instance_name(), "Selected scheduling not supported for these codes!");
			break;
		}

		/*
		 * Read the app_ram_ and store APP values in output_bits_llr_app() and
		 * hard decoded bits in output_bits buffer.
		 */
		Read_APP_RAM(app_ram_, iter, output_bits_llr_app(), output_bits());

		// Check whether all parity checks were satisfied in the previous iteration.
		last_iter = next_iter_is_last_iter;

		// Are all parity checks satisfied?
		if (pchk_satisfied == num_check_nodes_)
		{
			decoding_successful().Write(true);
			next_iter_is_last_iter = true; // Do one more iteration, as it is done in hardware!
		}

        // Store the number of flipped bits
        if (iter != 0) {
            flipped_bits(iter)().Write(Calc_Flipped_Bits(iter, output_bits()));
        }

		// Increase iteration counter.
		iter++;

		mean_iterations(iter)().Write(iter);


	/*
	 * Abort conditions:
	 * 1) maximum number of iterations is reached
	 * 2) all parity checks are satisfied: Since the hardware performs one more
	 * iteration after all parity checks are satisfied, we delay the stopping
	 * in the software as well.
	 */
	} while (iter < num_iterations() &&
	         last_iter == false);

	// Write the number of unsatisfied parity checks.
	num_unsatisfied_parity_checks().Write(num_check_nodes_ - pchk_satisfied);

	// Set number of used iterations in output buffer.
	iterations_performed().Write(iter);

	// Get statistic about modified bits.
	num_modified_systematic_bits().Write(Calc_Modified_Systematic_Bits(iter, input_bits_llr(), output_bits()));

	// Fill the output buffer and the status port for the remaining iterations.
	for(unsigned int i = iter; i < num_iterations(); i++)
	{
		mean_iterations(i + 1)().Write(iter);
		output_bits_llr_app()[i] = output_bits_llr_app()[iter - 1];
		output_bits()[i]         = output_bits()[iter - 1];
	}

	return 0;
}
}

