//
//  Copyright (C) 2010 - 2011 Creonic GmbH
//
//  This file is part of the Creonic simulation environment (CSE)
//  for communication systems.
//
/// \file
/// \brief  Hardware-compliant LDPC decoder shared functionality
/// \author Matthias Alles
/// \date   2010/12/03
//

#include <vector>
#include <assert.h>

#include "dec_ldpc_bin_hw_share.h"

/*
 * 0 : no logging
 * 1 : results on iteration level (parity checks failed)
 * 2 : results on check node group level (parity checks failed)
 * 3 : check node inputs / outputs
 */
#define LOG_LEVEL_LDPC 0
#define NLOGGING
//#include "debug_macros.h"
#include "../assistance/debug_macros.h"

namespace cse_lib {

void Decoder_LDPC_Binary_HW_Share::Change_Shift_Direction()
{
	for (u_int i = 0; i < max_check_degree_ * num_check_nodes_ / dst_parallelism_; i++)
		if (shft_vector_[i] > 0)
			shft_vector_[i] = dst_parallelism_ - shft_vector_[i];
}


void Decoder_LDPC_Binary_HW_Share::Init_APP_RAM(bool            parity_reordering,
                                                Buffer<int>    &input_bits_llr,
                                                Buffer<int, 2> &app_ram)
{
	unsigned int folding_factor;
	unsigned int num_info_nodes;
	unsigned int max_addr_normal_distribution;
	unsigned int i, j;

	folding_factor = src_parallelism_ / dst_parallelism_;
	num_info_nodes = num_variable_nodes_ - num_check_nodes_;

    // parity_reordering = is_IRA_code
	if (parity_reordering) {

		// For IRA codes we only distribute the information nodes vertically
		max_addr_normal_distribution = num_info_nodes / dst_parallelism_;

	} else {

		// For structured codes all bits are distributed the same
		max_addr_normal_distribution = num_variable_nodes_ / dst_parallelism_;

	}

	/*
	 * For each address distribute the channel values according to the decoder
	 * parallelism. For IRA codes this is done only for the systematic part
	 * like that.
	 */
	for(i = 0; i < max_addr_normal_distribution; i++)
		for(j = 0; j < dst_parallelism_; j++)
			app_ram[j][i] = input_bits_llr[
			                ((i / folding_factor) * dst_parallelism_ * folding_factor) +
			                 (j * folding_factor) +  // folding factor steps wide
			                 (i % folding_factor)];  // offset within submatrix

	if (parity_reordering) {

		// The parity nodes are distributed vertically for IRA codes!
		for(j = 0 ; j < dst_parallelism_; j++)
			for(i = 0; i < num_check_nodes_ / dst_parallelism_; i++)
				app_ram[j][i + num_info_nodes / dst_parallelism_] =
					    input_bits_llr[num_info_nodes +
					              i / folding_factor +
					              (i % folding_factor) * num_check_nodes_ / src_parallelism_ +
					              j * num_check_nodes_ / src_parallelism_ * folding_factor];
	}

	return;
}


void Decoder_LDPC_Binary_HW_Share::Read_APP_RAM(Buffer<int, 2>          &app_ram,
                                                int                      iter,
                                                Buffer<int, 2>          &output_bits_llr_app,
                                                Buffer<unsigned int, 2> &output_bits)
{
	unsigned int folding_factor;
	unsigned int num_info_nodes;
	unsigned int max_addr_normal_distribution;
	unsigned int i, j;

	folding_factor = src_parallelism_ / dst_parallelism_;
	num_info_nodes = num_variable_nodes_ - num_check_nodes_;


	if (is_IRA_code_) {

		// For IRA codes we only read the information nodes vertically
		max_addr_normal_distribution = num_info_nodes / dst_parallelism_;

	} else {

		// For structured codes all bits are distributed the same
		max_addr_normal_distribution = num_variable_nodes_ / dst_parallelism_;

	}

	/*
	 * For each address read the channel values according to the decoder
	 * parallelism. For IRA codes this is done only for the systematic part
	 * like that.
	 */
	for(i = 0; i < max_addr_normal_distribution; i++)
		for(j = 0; j < dst_parallelism_; j++)
			output_bits_llr_app[iter][ ((i / folding_factor) * dst_parallelism_ * folding_factor) +
			                                     (j * folding_factor) +
			                                     (i % folding_factor)] = app_ram[j][i];

	if (is_IRA_code_) {

		// The parity nodes are distributed vertically for IRA codes!
		for(j = 0 ; j < dst_parallelism_; j++)
			for(i = 0; i < num_check_nodes_ / dst_parallelism_; i++)
				output_bits_llr_app[iter][num_info_nodes +
				            i / folding_factor +
				           (i % folding_factor) * num_check_nodes_ / src_parallelism_ +
				            j * num_check_nodes_ / src_parallelism_ * folding_factor] =
				          app_ram[j][i + num_info_nodes / dst_parallelism_];
	}

	// Calculate the sign bit.
	for(i = 0; i < num_variable_nodes_ ; i++)
		output_bits[iter][i] = (output_bits_llr_app[iter][i] < 0 ? 1 : 0);

	return;

}


void Decoder_LDPC_Binary_HW_Share::Get_Check_Node_Input(Buffer<int, 2> &app_ram,
                                                        Buffer<int, 2> &msg_ram,
                                                        unsigned int    iter,
                                                        unsigned int    cng_counter,
                                                        unsigned int    cfu_counter,
                                                        Buffer<int>    &check_node_in,
                                                        Buffer<int>    &app_out)
{
    int temp;
	int vng_select;
	int shift_value;
	int app_ram_content;
	unsigned int vn_select;
	unsigned int vector_addr;
    // necessary for allocating the previous_check_node_in (zeta i - 1)
    unsigned int num_cng = num_check_nodes_ / dst_parallelism_; 


    // one dimensional array for storing all the previous iteration zeta (zeta i -1)
    // is dinamically allocated because, VLA can't have static qualifier
    static int* previous_check_node_in = new int[num_cng * dst_parallelism_ * max_check_degree_];

    /*
    static Buffer<int, 3> previous_check_node_in;
	unsigned int dimensions[3];
	dimensions[0] = num_cng;
	dimensions[1] = dst_parallelism_;
	dimensions[2] = max_check_degree_;
    if (iter == 0) {
        previous_check_node_in.Resize(dimensions);
    }
    */


	// Calculate the base address for addressing shift and address vectors.
	vector_addr = cng_counter * max_check_degree_;

	// Iterate over the single edges.
	for(unsigned int vn2cn_msg = 0; vn2cn_msg < max_check_degree_; vn2cn_msg++)
	{
		// Determine the variable node group we want to process and its shift value.
		vng_select = addr_vector_[vector_addr];
		shift_value = shft_vector_[vector_addr];

		// Determine the variable node within the current variable node group.
		vn_select = (shift_value + cfu_counter) % dst_parallelism_;

		/*
		 * Calulate the input for the check nodes here. Only if the
		 * address to the variable node group is valid, we have to
		 * read the app_ram. Otherwise we insert the neutral element
		 * with respect to the check node operation (= maximum
		 * message).
		 */
		if (vng_select > -1)
		{
			app_ram_content = app_ram[vn_select][vng_select];

			// Calculate check node input.
            // if not first iteration
			if (iter != 0) {

                /* if Min_Sum_Self_Correcting
                 * Substract extrinsic info from previous iteration to app from previous iteration
                 * but this time store it in temp and compare sign of temp with sign of previous message
                 * if equal, then store new message, if not, then store a 0
                 * Note: Whenever the old mesage == 0, we update the new message 
                 */
                if (check_node_algorithm_ == MIN_SUM_SELF_CORRECTING) {
                    temp = app_ram_content - msg_ram[vn_select][vector_addr];
                    if (Get_Sign(temp) == Get_Sign(previous_check_node_in[cng_counter * dst_parallelism_ * max_check_degree_ + cfu_counter * max_check_degree_ + vn2cn_msg]) || previous_check_node_in[cng_counter * dst_parallelism_ * max_check_degree_ + cfu_counter * max_check_degree_ + vn2cn_msg] == 0) {
                        check_node_in[vn2cn_msg] = temp;
                    } else {
                        check_node_in[vn2cn_msg] = 0;
                    }
                    // either way, me must store the zeta of this iteration for comparison with next
                    // constants are derived from manual calc
                    previous_check_node_in[cng_counter * dst_parallelism_ * max_check_degree_ + cfu_counter * max_check_degree_ + vn2cn_msg] = check_node_in[vn2cn_msg];
                }
                // if normal Min_Sum
                 else {
                    // Subtract extrinsic information from previous iteration.
                    check_node_in[vn2cn_msg] = app_ram_content - msg_ram[vn_select][vector_addr];
                }
            }
            // if first iteration
             else {

				// In the first iteration read only the channel information.
				check_node_in[vn2cn_msg] = app_ram_content;
                // if Min_Sum_Self_Correcting
                // store initial zeta for comparison with next iteration's zeta
                if (check_node_algorithm_ == MIN_SUM_SELF_CORRECTING) {
                    previous_check_node_in[cng_counter * dst_parallelism_ * max_check_degree_ 
                        + cfu_counter * max_check_degree_ + vn2cn_msg] = check_node_in[vn2cn_msg];
                }
            }

			app_out[vn2cn_msg] = app_ram_content;
		}
		else
		{
			//printf("vn2cn_msg: %u\r\n", vn2cn_msg);
			// No valid address => Insert value that does not affect the check node.
			check_node_in[vn2cn_msg] = max_msg_extr_;

			app_out[vn2cn_msg] = 0;
		}

		if(is_IRA_code_     &&
		   cfu_counter == 0 &&
		   shift_value == (signed) dst_parallelism_ - 1 &&
		   vng_select  == (signed) (num_variable_nodes_ / dst_parallelism_ - 1))
		{
			check_node_in[vn2cn_msg] = static_cast<int> (max_msg_extr_);
			app_out[vn2cn_msg] = 0;
		}

		/*
		 * Saturate the input to the check node, symmetrically due to signed-magnitude
		 * representation in hardware.
		 */
		Saturate_Value(check_node_in[vn2cn_msg], max_msg_extr_);

		// Update to next edge.
		vector_addr++;
	}

	LM_OUT_LEVEL(LDPC, 3, "CNG: " << cng_counter <<
	                      " CFU: " << cfu_counter <<
	                      " APP RAM shifted out: " << app_out << endl);

}


void Decoder_LDPC_Binary_HW_Share::Write_Check_Node_Output(Buffer<int, 2> &app_ram,
                                                           Buffer<int, 2> &msg_ram,
                                                           unsigned int iter,
                                                           unsigned int cng_counter,
                                                           unsigned int cfu_counter,
                                                           Buffer<int>  &check_node_out)
{
	int vng_select;
	int shift_value;
	unsigned int vn_select;
	unsigned int vector_addr;

	// Calculate the base address for shift and address vectors.
	vector_addr = cng_counter * max_check_degree_;

	// Process each edge of the check node separately.
	for(unsigned int cn2vn_msg = 0; cn2vn_msg < max_check_degree_; cn2vn_msg++)
	{
		// Determine the variable node group we want to process.
		vng_select = addr_vector_[vector_addr];
		shift_value = shft_vector_[vector_addr];

		// Determine the variable node within the current variable node group.
		vn_select = (shift_value + cfu_counter) % dst_parallelism_;

		// In case of IRA codes set the output for virtual edge to 0.
		if(is_IRA_code_     &&
		   cfu_counter == 0 &&
		   shift_value == (signed) dst_parallelism_ - 1 &&
		   vng_select  == (signed) (num_variable_nodes_ / dst_parallelism_ - 1))
		{
			check_node_out[cn2vn_msg] = 0;
		}

		// In case of a valid variable node group, update app_ram and msg_ram.
		if(vng_select > -1)
		{
            // at this momement check_node_out = messages from check nodes to variable nodes (epsilon)
			int current_app_value = app_ram[vn_select][vng_select];
			int current_message   = check_node_out[cn2vn_msg];

            // tempAPP = lambda channel + epsilon of that iteration for first iteration
            // or tempAPP = app(i -1) + epsilon(i)
			current_app_value += current_message;

            // if it's not the first iteration
            // we must substract epsilon of previous iteration to get =>  APP(i) = app(i-1) + epsilon(i) - epsilon(i-1) = appchannel + epsilon(i) 
            // and in this way updating APP(i-1) to APP(i)
			if (iter != 0)
				current_app_value -= msg_ram[vn_select][vector_addr];

			/*
			 * Saturate the APP value. In contrast to the input check node messages,
			 * there is no need to saturate symmetrically, since this value will
			 * never be represented as a signed-magnitude value.
			 */
			Saturate_Value(current_app_value, max_msg_app_, - max_msg_app_ - 1);

			// Write back results
            // we are saying: APP = tempAPP
            // msg_ram = epsilon(i)
			app_ram[vn_select][vng_select] = current_app_value;
			msg_ram[vn_select][vector_addr] = current_message;
		}

		vector_addr++;
	}
}


unsigned int Decoder_LDPC_Binary_HW_Share::Calc_Parity_Check(Buffer<int> &values)
{

	unsigned int parity_check = 0;

	for(unsigned int i = 0; i < max_check_degree_; i++)
		if (values[i] < 0)
			parity_check++;

	parity_check %= 2;

	return parity_check;
}


/*********************
 * Decoder Functions *
 *********************/


unsigned int Decoder_LDPC_Binary_HW_Share::Decode_Layered(Buffer<int, 2> &app_ram,
                                                          Buffer<int, 2> &msg_ram,
                                                          int             iter)
{
	Buffer<int> check_node_io(max_check_degree_);
	Buffer<int> app_out(max_check_degree_);
	cn_msg_abs_.Resize(max_check_degree_);
	cn_msg_sign_.Resize(max_check_degree_);
	unsigned int ok_checks = 0;

	// Iterate over all check node groups.
	for(unsigned int cng_counter = 0; cng_counter < num_check_nodes_ / dst_parallelism_; cng_counter++)
	{
		// Iterate over the functional units.
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
		{
			Get_Check_Node_Input(app_ram, msg_ram, iter, cng_counter, cfu_counter, check_node_io, app_out);

			ok_checks += (1 - Calc_Parity_Check(app_out));

			switch(check_node_algorithm_)
			{
			case MIN_SUM:
            case MIN_SUM_SELF_CORRECTING:
				Check_Node_Min_Sum(check_node_io, esf_factor_); break;
			case LAMBDA_MIN:
				Check_Node_Lambda_Min(check_node_io, num_lambda_min_, bw_fract_); break;
            case SPLIT_ROW:
            case SPLIT_ROW_IMPROVED:
            case SPLIT_ROW_SELF_CORRECTING:
                Check_Node_Split_Row(check_node_io, esf_factor_, num_partitions_, threshold_, cng_counter); break;
			}

			Write_Check_Node_Output(app_ram, msg_ram, iter, cng_counter, cfu_counter, check_node_io);
		}
	}

	return ok_checks;
}


unsigned int Decoder_LDPC_Binary_HW_Share::Decode_Layered_Superposed(Buffer<int, 2> &app_ram,
                                                                     Buffer<int, 2> &msg_ram,
                                                                     int             iter)
{
	Buffer<int, 2> check_node_io(dst_parallelism_, max_check_degree_);
	Buffer<int> app_out(max_check_degree_);
	cn_msg_abs_.Resize(max_check_degree_);
	cn_msg_sign_.Resize(max_check_degree_);
	unsigned int ok_checks = 0;

	// Iterate over all check node groups.
	for(unsigned int cng_counter = 0; cng_counter < num_check_nodes_ / dst_parallelism_; cng_counter++)
	{
		// Read the input for a whole check node group first.
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
		{
			Get_Check_Node_Input(app_ram,
			                     msg_ram,
			                     iter,
			                     cng_counter,
			                     cfu_counter,
			                     check_node_io[cfu_counter],
			                     app_out);

			ok_checks += (1 - Calc_Parity_Check(app_out));
		}

		LM_OUT_LEVEL(LDPC, 2, "CNG: " << cng_counter <<
		                      " Checks failed: " << dst_parallelism_ * (1 + cng_counter) - ok_checks << endl);

		// Perform the Check Node calculations for all CFUs now.
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
		{
			LM_OUT_LEVEL(LDPC, 3, "CNG: " << cng_counter <<
			                      " CFU: " << cfu_counter <<
			                      " in: " << check_node_io[cfu_counter] << endl);

			switch(check_node_algorithm_)
			{
			case MIN_SUM:
            case MIN_SUM_SELF_CORRECTING:
				Check_Node_Min_Sum(check_node_io[cfu_counter], esf_factor_);
				break;

			case LAMBDA_MIN:
				Check_Node_Lambda_Min(check_node_io[cfu_counter], num_lambda_min_, bw_fract_);
				break;
                // all of the Split Row Algorithms are Split Row Threshold
            case SPLIT_ROW:
            case SPLIT_ROW_IMPROVED:
            case SPLIT_ROW_SELF_CORRECTING:
                Check_Node_Split_Row(check_node_io[cfu_counter], esf_factor_, num_partitions_, threshold_, cng_counter);
                break;
			}

			LM_OUT_LEVEL(LDPC, 3, "CNG: " << cng_counter <<
			                      " CFU: " << cfu_counter <<
			                      " out: " << check_node_io[cfu_counter] << endl);
		}

		// Write back the result of all check nodes.
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
			Write_Check_Node_Output(app_ram,
			                        msg_ram,
			                        iter,
			                        cng_counter,
			                        cfu_counter,
			                        check_node_io[cfu_counter]);
	}

	LM_OUT_LEVEL(LDPC, 1, "Iter: " << iter << " Checks failed: " << num_check_nodes_ - ok_checks << endl);

	return ok_checks;
}


unsigned int Decoder_LDPC_Binary_HW_Share::Decode_Two_Phase(Buffer<int, 2> &app_ram,
                                                            Buffer<int, 2> &msg_ram,
                                                            int             iter,
                                                            bool            app_parity_check)
{
	// Number of check node groups
	unsigned int num_cng = num_check_nodes_ / dst_parallelism_;

	Buffer<int, 3> check_node_io;
	unsigned int dimensions[3];
	dimensions[0] = num_cng;
	dimensions[1] = dst_parallelism_;
	dimensions[2] = max_check_degree_;
	check_node_io.Resize(dimensions);
	Buffer<int> app_out(max_check_degree_);
	cn_msg_abs_.Resize(max_check_degree_);
	cn_msg_sign_.Resize(max_check_degree_);
	unsigned int ok_checks = 0;

	// Iterate over all check nodes.
	for(unsigned int cng_counter = 0; cng_counter < num_cng; cng_counter++)
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
		{
			Get_Check_Node_Input(app_ram,
			                     msg_ram,
			                     iter,
			                     cng_counter,
			                     cfu_counter,
			                     check_node_io[cng_counter][cfu_counter],
			                     app_out);

			if (app_parity_check)
				ok_checks += (1 - Calc_Parity_Check(app_out));
			else
				ok_checks += (1 - Calc_Parity_Check(check_node_io[cng_counter][cfu_counter]));
		}

	// Perform the Check Node calculations for all CFUs now.
	for(unsigned int cng_counter = 0; cng_counter < num_cng; cng_counter++) {
        // cout << cng_counter << " row" << endl;
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
			switch(check_node_algorithm_)
			{
			case MIN_SUM:
            case MIN_SUM_SELF_CORRECTING:
				Check_Node_Min_Sum(check_node_io[cng_counter][cfu_counter], esf_factor_);
				break;

			case LAMBDA_MIN:
				Check_Node_Lambda_Min(check_node_io[cng_counter][cfu_counter],
				                      num_lambda_min_,
				                      bw_fract_);
				break;
                // all of the Split Row Algorithms are Split Row Threshold
            case SPLIT_ROW:
            case SPLIT_ROW_IMPROVED:
            case SPLIT_ROW_SELF_CORRECTING:
                Check_Node_Split_Row(check_node_io[cng_counter][cfu_counter], esf_factor_, num_partitions_, threshold_, cng_counter);
                break;
			}
    }

	// Write back the result of all check nodes.
	for(unsigned int cng_counter = 0; cng_counter < num_cng; cng_counter++)
		for(unsigned int cfu_counter = 0; cfu_counter < dst_parallelism_; cfu_counter++)
			Write_Check_Node_Output(app_ram,
			                        msg_ram,
			                        iter,
			                        cng_counter,
			                        cfu_counter,
			                        check_node_io[cng_counter][cfu_counter]);

	return ok_checks;
}



/************************
 * Check Node Functions *
 ************************/

int Decoder_LDPC_Binary_HW_Share::Check_Node_Lambda_Min(Buffer<int>  &in_out_msg,
                                                        unsigned int  num_lambda_min,
                                                        unsigned int  num_bits_fract)
{
	unsigned int check_node_degree = in_out_msg.length();
	unsigned int min_msg[4];
	unsigned int min_idx[4];
	int pchk_sign = 1;
	unsigned int minstar_msg;
	unsigned int index = 0;

	// Separate magnitude and sign.
	for (unsigned int i = 0; i < check_node_degree; i++)
	{
		cn_msg_abs_[i]  = abs(in_out_msg[i]);
		cn_msg_sign_[i] = (in_out_msg[i] >= 0) ? 1 : -1;
		pchk_sign *= cn_msg_sign_[i];
	}

	// Find the num_lambda_min() minima.
	for(unsigned int i = 0; i < num_lambda_min; i++)
	{
		// Find the current minimum and remember its magnitude and index.
		min_msg[i] = Minimum_Fixp(cn_msg_abs_, index);
		min_idx[i] = index;

		// Replace the current minimum and continue with next minimum search.
		cn_msg_abs_[index] = max_msg_extr_ + 1;
	}

	// 2-Min algorithm
	if (num_lambda_min == 2)
		for(unsigned int i = 0; i < check_node_degree; i++)
		{
			if(min_idx[0] == i)
				minstar_msg = min_msg[1];

			else if(min_idx[1] == i)
				minstar_msg = min_msg[0];

			else
				minstar_msg = Minstar_Fixp(min_msg[0], min_msg[1], num_bits_fract);

			// Calculate new outgoing edge with new sign and new magnitude.
			in_out_msg[i] = minstar_msg * cn_msg_sign_[i] * pchk_sign;
		}

	// 3-Min algorithm
	else if (num_lambda_min == 3)
		for(unsigned int i = 0; i < check_node_degree; i++)
		{
			if(min_idx[0] == i)
				minstar_msg = Minstar_Fixp(min_msg[1], min_msg[2], num_bits_fract);

			else if(min_idx[1] == i)
				minstar_msg = Minstar_Fixp(min_msg[0], min_msg[2], num_bits_fract);

			else if(min_idx[2] == i)
				minstar_msg = Minstar_Fixp(min_msg[0], min_msg[1], num_bits_fract);

			else
				minstar_msg = Minstar_Fixp(Minstar_Fixp(min_msg[0], min_msg[1], num_bits_fract),
				                           min_msg[2],
				                           num_bits_fract);

			// Calculate new outgoing edge with new sign and new magnitude.
			in_out_msg[i] = minstar_msg * cn_msg_sign_[i] * pchk_sign;
		}
	// Return whether parity check was satisfied or not.
	return pchk_sign;
}


int Decoder_LDPC_Binary_HW_Share::Check_Node_Min_Sum(Buffer<int> &in_out_msg, float esf_factor)
{
	unsigned int check_node_degree = in_out_msg.length();
	unsigned int min0_msg, min1_msg;
	unsigned int min0_idx;
	int  pchk_sign = 1;
	unsigned int min_out_msg;
	unsigned int index = 0;

    // cout << "input" << endl;
    for (unsigned int i = 0; i != check_node_degree; i++) {
        // cout << "in_out_msg[" << i << "] = " << in_out_msg[i] << endl;
    }
    // cout << endl;

	// Separate magnitude and sign.
	for (unsigned int i = 0; i < check_node_degree; i++)
	{
		cn_msg_abs_[i]  = abs(in_out_msg[i]);
		cn_msg_sign_[i] = (in_out_msg[i] >= 0) ? 1 : -1;
		pchk_sign *= cn_msg_sign_[i];
	}

	// Find the current minimum and remember its magnitude and index.
	min0_msg = Minimum_Fixp(cn_msg_abs_, index);
	min0_idx = index;

	// Replace the current minimum and continue with next minimum search.
	cn_msg_abs_[index] = max_msg_extr_;

	// Find the second minimum and remember its magnitude and index.
	min1_msg = Minimum_Fixp(cn_msg_abs_, index);

	if(esf_factor == 0.875)
	{
		// Multiply with 0.875 like it is done in the hardware.
		min0_msg = ( ((min0_msg << 1) +
		              (min0_msg << 0) +
		              (min0_msg >> 1) + 1) >> 2);
		min1_msg = ( ((min1_msg << 1) +
		              (min1_msg << 0) +
		              (min1_msg >> 1) + 1) >> 2);
	}
	else
	{
		// Multiply with 0.75 like it is done in the hardware.
		min0_msg = ( ((min0_msg << 1) + (min0_msg << 0) + 1) >> 2);
		min1_msg = ( ((min1_msg << 1) + (min1_msg << 0) + 1) >> 2);
	}

    // cout << "output" << endl;
	for(unsigned int i = 0; i < check_node_degree; i++)
	{
		if(min0_idx == i)
			min_out_msg = min1_msg;
		else
			min_out_msg = min0_msg;

		// Calculate new outgoing edge with new sign and new magnitude.
        // converted again to 2's complement
		in_out_msg[i] = min_out_msg * cn_msg_sign_[i] * pchk_sign;
        // cout << "in_out_msg[" << i << "] = " << in_out_msg[i] << endl;
	}
    // cout << endl;

	return pchk_sign;
}

// Check_Node_Split_Row: implements Check_Node functionality according to Split-Row Treshold and Split-Row Threshold Improved
int Decoder_LDPC_Binary_HW_Share::Check_Node_Split_Row(Buffer<int> &in_out_msg, float esf_factor, unsigned int partitions, unsigned int threshold, unsigned int cng_counter)
{

    std::vector< std::vector<int> > parts;      // vector that holds partitions
    std::vector< std::vector<unsigned int> > temp_parts;      // vector that holds partitions temporarily
    parts.resize(partitions);
    temp_parts.resize(partitions);
    int sign_part[partitions];               // array that holds accumulated sign of each partition
    unsigned int local_min1[partitions];     // array that holds min1 for each partition
    unsigned int local_min2[partitions];     // array that holds min2 for each partition
    unsigned int min1_idx[partitions];       // array that hold index of min1 for each partition
    std::vector<unsigned int> threshold_en(partitions, 0);   // vector that holds the threshold_en signal for each partition
    int pchk_sign = 1;
    unsigned int partition_length[partitions]; // array that holds length of each partition
    unsigned int part_length;

    // cout << "input" << endl;
    /*
    for (unsigned int i = 0; i != in_out_msg.length(); i++) {
        // cout << "in_out_msg[" << i << "] = " << in_out_msg[i] << endl;
    }
    */
    // cout << endl << endl;

    // Calculate each partition length
    Calculate_Partition_Length(partition_length, cng_counter, partitions);

    /*
    for (unsigned int i = 0; i != partitions; i++) {
        // cout << "partition_length[" << i << "] = " << partition_length[i] << endl;
    }
    */
    // cout << endl;

    // distribute the messages between the partitions
    Distribute_Messages_Partitions(parts, in_out_msg, partitions, partition_length);

    /*
    for (unsigned int i = 0; i != partitions; i++) {
        for (unsigned int j = 0; j != partition_length[i]; j++) {
            // cout << "parts[" << i << "][" << j << "] = " << parts[i][j] << endl;
        }
    }
    */
    // cout << endl;

    // find local first and second minimum, signs and set threshold_enable for each partition
    for (unsigned int part_num = 0; part_num != partitions; part_num++) {
        
        // for ensuring we add the length of the previous partition for correct indexing of cn_msg_sign_ and cn_msg_abs_
        if (part_num == 0) {
            part_length = 0;
        } else {
            part_length = partition_length[part_num - 1];
        }
        sign_part[part_num] = Split_Row_Local_Minimum_Sign(parts[part_num], local_min1[part_num], min1_idx[part_num], local_min2[part_num], threshold_en[part_num], threshold, part_length);
        // cout << "local_min1[" << part_num << "] = " << local_min1[part_num] << endl;
        // cout << "local_min2[" << part_num << "] = " << local_min2[part_num] << endl;
        // cout << "min1_idx[" << part_num << "] = " << min1_idx[part_num] << endl;
        // cout << "threshold_en[" << part_num << "] = " << threshold_en[part_num] << endl;
        // cout << "sign_part[" << part_num << "] = " << sign_part[part_num] << endl;
    }
    // cout << endl;

    /*
    for (unsigned int i = 0; i != in_out_msg.length(); i++) {
        // cout << "cn_msg_sign_[" << i << "] = " << cn_msg_sign_[i] << endl;
    }
    */
    // cout << endl;

    // apply Split Row Threshold Improved or Split Row Threshold (Split Row Self Correcting as well) algorithm
    if (check_node_algorithm_ == SPLIT_ROW_IMPROVED || check_node_algorithm_ == SPLIT_ROW_SELF_CORRECTING) {
        for (unsigned int part_num = 0; part_num != partitions; part_num++) {
            
            // for ensuring we add the length of the previous partition for indexing cn_msg_sign and cn_msg_abs (used in Multiply_Sign_Esf)
            if (part_num == 0) {
                part_length = 0;
            } else {
                part_length = partition_length[part_num - 1];
            }

            if (local_min1[part_num] <= threshold && local_min2[part_num] <= threshold) {
                // condition 1
                // cout << "condition 1" << endl;
                Update_Minimum(temp_parts[part_num], min1_idx[part_num], local_min1[part_num], local_min2[part_num], partition_length[part_num]);
            } else if (local_min1[part_num] <= threshold && local_min2[part_num] > threshold) {
                // condition 2
                // cout << "condition 2" << endl;
                if (Neighbors_Threshold_Enable(part_num, partitions, threshold_en)) {
                    // condition 2a
                    // cout << "condition 2a" << endl;
                    Update_Minimum(temp_parts[part_num], min1_idx[part_num], local_min1[part_num], threshold, partition_length[part_num]);
                } else {
                    // condition 2b
                    // cout << "condition 2b" << endl;
                    Update_Minimum(temp_parts[part_num], min1_idx[part_num], local_min1[part_num], local_min2[part_num], partition_length[part_num]);
                }
            } else if (local_min1[part_num] > threshold && Neighbors_Threshold_Enable(part_num, partitions, threshold_en)) {
                // condition 3
                // cout << "condition 3" << endl;
                Update_Minimum(temp_parts[part_num], min1_idx[part_num], threshold, threshold, partition_length[part_num]);
            } else {
                // condition 4
                // cout << "condition 4" << endl;
                Update_Minimum(temp_parts[part_num], min1_idx[part_num], local_min1[part_num], local_min2[part_num], partition_length[part_num]);
            }
            Multiply_Sign_Esf(parts[part_num], temp_parts[part_num], sign_part[part_num], esf_factor, part_length);
        }

    } else {
        for (unsigned int part_num = 0; part_num != partitions; part_num++) {

            // for ensuring we add the length of the previous partition for indexing 
            if (part_num == 0) {
                part_length = 0;
            } else {
                part_length = partition_length[part_num - 1];
            }

            if (local_min1[part_num] > threshold && Neighbors_Threshold_Enable(part_num, partitions, threshold_en)) {
                // condition 3
                // cout << "condition 3 of split row threshold" << endl;
                Update_Minimum(temp_parts[part_num], min1_idx[part_num], threshold, threshold, partition_length[part_num]);
            } else {
                // condition 1 and 4
                // cout << "condition 1 and 4 of split row threshold" << endl;
                Update_Minimum(temp_parts[part_num], min1_idx[part_num], local_min1[part_num], local_min2[part_num], partition_length[part_num]);
            }
            // multiply min with local sign and own sign
            Multiply_Sign_Esf(parts[part_num], temp_parts[part_num], sign_part[part_num], esf_factor, part_length);
        }
    }

    // Also consider the sign of the adjacent partitions
    for (unsigned int part_num = 1; part_num != partitions; part_num++) {
        Multiply_Split_Sign_Signal(parts[part_num - 1], sign_part[part_num]);
        Multiply_Split_Sign_Signal(parts[part_num], sign_part[part_num - 1]);
        if (part_num == 1) {
            // if is the first time, then multiply the sign of first and second partition
            // else keep multiply new partition sign with the accumulated one
            pchk_sign = sign_part[part_num - 1] * sign_part[part_num];
        } else {
            pchk_sign *= sign_part[part_num];
        }
    }

    // put everything in in_out_msg so it can be read in the variable node again
    Regroup_Messages_Partitions(parts, in_out_msg, partition_length);

    return pchk_sign;
}

// Calculate Partition Length: Calculates for each partitions its lenght according to the number of valid paririty check matrix values
void Decoder_LDPC_Binary_HW_Share::Calculate_Partition_Length(unsigned int partition_length[], unsigned int cng_counter, unsigned int partitions)
{
    unsigned int rest;
    unsigned vector_addr;
    int vng_select;

    // 1st: Check how many valid entries are there in the parity check matrix "row" (from base address)

    // Calculate the base address for addressing shift and address vectors.
    vector_addr = cng_counter * max_check_degree_;
    unsigned int count = 0;
    for (unsigned int i = 0; i != max_check_degree_; i++) {
        // Determine the variable node group we want to process
        vng_select = addr_vector_[vector_addr];
        // if address to the variable node group is valid
        if (vng_select != -1) {
            count++;
        }
        vector_addr++;
    }

    rest = max_check_degree_ - count; // the invalid elements of the parity_check_matrix

    // 2nd: Store length of each partition equitably w.r.t valid elements only, and include invalid elements for last partition
    if (count % partitions == 0) {
        // if length is a multiple of partitions then they are perfectly divisible
        for (unsigned int i = 0; i != partitions - 1; i++) {
            partition_length[i] = count / partitions;
        }
        // the last partitions should cover the rest of the invalid parity check matrix elements
        partition_length[partitions - 1] = (count / partitions) + rest;
    } else {
        // if they're not perfectly divisible, make first partition one input bigger than rest of partitions
        partition_length[0] = (count / partitions) + 1;
        // if partitions are 4, 8 or 16 then fill intermediate partitions' length
        for (unsigned int i = 1; i != partitions - 1; i++) {
            partition_length[i] = count / partitions;
        }
        // the last partitions should cover the rest of the invalid parity check matrix elements
        partition_length[partitions - 1] = (count / partitions) + rest;
    }
}


// Distribute_Messages_Partitions: distributes messages from variable node (in_out_msg) between partitions
void Decoder_LDPC_Binary_HW_Share::Distribute_Messages_Partitions(std::vector< std::vector<int> > &parts, Buffer<int> &in_out_msg, unsigned int partitions, unsigned int partition_length[])
{
    // we need to change this function for variable size partitions
    unsigned int last_pos = 0;
    for (unsigned int i = 0; i != partitions; i++) {
        for (unsigned int j = 0; j != partition_length[i]; j++) {
            parts[i].push_back(in_out_msg[last_pos + j]);
        }
        last_pos += partition_length[i];
    }
   
}


// Split_Row_Local_Minimum_Sign: gets local minima (1st and 2nd), gets index of 1st minima, sets threshold_en signal, and calculates split_row_sign signal of all msg per partition
int Decoder_LDPC_Binary_HW_Share::Split_Row_Local_Minimum_Sign(std::vector<int>& part, unsigned int& minimum1, unsigned int& min1_idx, unsigned int& minimum2, unsigned int& threshold_en, unsigned int threshold, unsigned int part_length)
{

    unsigned int check_node_degree = part.size();
	int  pchk_sign = 1;
	unsigned int index = 0;

	// Separate magnitude and sign.
	for (unsigned int i = 0; i != check_node_degree; i++)
	{
        // we need part_num in order to store them in a different place from other partitions (remember multiple partitions)
        // we are storing all magnitudes and signs of each edge in one buffer, but we use indexing to access correct one for each partition
        // part does not need additional part_num * check_node_degree because this is the vector for that partition (vector<vector<unsigned int> > is taken care of in caller) 
		cn_msg_abs_[part_length + i]  = abs(part[i]);
		cn_msg_sign_[part_length + i] = (part[i] >= 0) ? 1 : -1;
		pchk_sign *= cn_msg_sign_[part_length + i];
	}

	// Find the current minimum and remember its magnitude and index.
	minimum1 = Minimum_Fixp_Split_Row(cn_msg_abs_, index, part_length, check_node_degree);
	min1_idx = index; // Update_Min searches in that partition from 0 to len = (in_out_msg.length() / partitions)

	// Replace the current minimum and continue with next minimum search.
	cn_msg_abs_[part_length + index] = max_msg_extr_;

	// Find the second minimum and remember its magnitude and index.
	minimum2 = Minimum_Fixp_Split_Row(cn_msg_abs_, index, part_length, check_node_degree);

    // store minimums
    // in the case of split row, this is done in function Update_minimum

    // if local min <= threshold, then threshold_en = 1
    if (minimum1 <= threshold) {
        threshold_en = true;
    } else {
        threshold_en = false;
    }
    return pchk_sign;
}


//bool Decoder_LDPC_Binary_HW_Share::Neighbors_Threshold_Enable(unsigned int part_num, unsigned int partitions, unsigned int threshold_en[])
// Neighbors_Threshold_Enable: returns true if any of the neigboring partitions has threshold_en asserted, else false
bool Decoder_LDPC_Binary_HW_Share::Neighbors_Threshold_Enable(unsigned int part_num, unsigned int partitions, std::vector<unsigned int>& threshold_en)
{
    if (part_num == 0) {
        return threshold_en[1];
    } else if (part_num == partitions - 1) {
        return threshold_en[part_num - 1];
    } else {
        return threshold_en[part_num - 1] || threshold_en[part_num + 1];
    }
}

// Update_Minimum: stores the magnitude of that splitted row
void Decoder_LDPC_Binary_HW_Share::Update_Minimum(std::vector<unsigned int>& temp_part, unsigned int min1_idx, unsigned int min1, unsigned int min2, unsigned int len)
{
    unsigned int min_out_msg;
    unsigned int check_node_degree = len;

    for (unsigned int i = 0; i != check_node_degree; i++) {
        if (i == min1_idx) {
            min_out_msg = min2;
        } else {
            min_out_msg = min1;
        }
        temp_part.push_back(min_out_msg);
        // cout << "temp_part[" << i << "] = " << temp_part[i] << endl;
    }
    // cout << endl;
}

// Multiply_Sign_Esf: Multiplies magnitude and local sign and esf factor and stores it in signed representation (vector <> part)
void Decoder_LDPC_Binary_HW_Share::Multiply_Sign_Esf(std::vector<int>& part, std::vector<unsigned int>& temp_part, int local_pchk_sign, float esf_factor, unsigned int part_length)
{
    assert(part.size() == temp_part.size());
    unsigned int check_node_degree = part.size();
	for(unsigned int i = 0; i < check_node_degree; i++)
	{
        if (esf_factor == 0.875)
        {
            // Multiply with 0.875 like it is done in the hardware.
            temp_part[i] = (((temp_part[i] << 1) + 
                        (temp_part[i] << 0) + 
                        (temp_part[i] >> 1) + 1) >> 2);
        }
        else if (esf_factor == 0.75)
        {
            // Multiply with 0.75 like it is done in the hardware.
            temp_part[i] = ( ((temp_part[i] << 1) + (temp_part[i] << 0) + 1) >> 2);
        }

        // Calculate new outgoing edge with new sign and new magnitude.
        // we need j in order to multiply it with correct sign (remember multiple partitions)
		part[i] = temp_part[i] * cn_msg_sign_[part_length + i] * local_pchk_sign;
        // cout << "part[" << i << "] = " << part[i] << endl;
	}
    // cout << endl;
}

// Multiply_Split_Sign_Signal: Multiply messages of each partition with the sign of the neighboring partition and store it as message
void Decoder_LDPC_Binary_HW_Share::Multiply_Split_Sign_Signal(std::vector<int>& part, int split_sign)
{
    unsigned int check_node_degree = part.size();

    for (unsigned int i = 0; i != check_node_degree; i++) {
        part[i] *= split_sign;
        // cout << "part[" << i << "] = " << part[i] << endl;
    }
    // cout << endl;
}

// Regroup_Messages_Partitions: Regroups messages of all partitions in Buffer<in_out_msg> used in Write_Check_Node_Output
void Decoder_LDPC_Binary_HW_Share::Regroup_Messages_Partitions(std::vector< std::vector<int> > &parts, Buffer<int> &in_out_msg, unsigned int partition_length[])
{
    // cout << "output" << endl;
    for (unsigned int i = 0, j = 0, k = 0; i != in_out_msg.length(); i++, k++) {
        /* 
         * if i reaches the lenght of a row of a partition
         * then we have finished reading this partition,
         * so we grab the next partition (increment j)
         */
        if (k != 0 && k == partition_length[j]) {
            j++;
            k = 0;
        }
        in_out_msg[i] = parts[j][k];
        // cout << "in_out_msg[" << i << "] = " << in_out_msg[i] << endl;
    }
    // cout << endl;

}

// Minimum_Fixp_Split_Row: finds minima and its index in the messages of a partition
unsigned int Decoder_LDPC_Binary_HW_Share::Minimum_Fixp_Split_Row(Buffer<unsigned int> &message, unsigned int& idx, unsigned int part_length, unsigned int check_node_degree)
{
    unsigned int temp = max_msg_extr_;

    // here check_node_degree = len = length of a partition
	for(unsigned int i = 0; i != check_node_degree; i++) {
		if (message[part_length + i] <= temp) {
			idx  = i;
			temp = message[part_length+ i];
		}
    }

	return temp;
}

unsigned int Decoder_LDPC_Binary_HW_Share::Minimum_Fixp(Buffer<unsigned int> &message, unsigned int &idx)
{
	unsigned int temp = max_msg_extr_;

	for(unsigned int i = 0; i < max_check_degree_; i++) {
		if(message[i] <= temp) {
			idx  = i;
			temp = message[i];
		}
    }

	return temp;
}


inline int Decoder_LDPC_Binary_HW_Share::Delta(int x, unsigned int num_bits_fract)
{
	int d = ((5 << (num_bits_fract + 2 - 3)) - (x)) >> 2;

	return (d > 0) ? d : 0;
}


inline int Decoder_LDPC_Binary_HW_Share::Minstar_Fixp(int a, int b, unsigned int num_bits_fract)
{
	int min = (a < b) ? a : b;
	int d = - Delta( abs(a - b), num_bits_fract ) + Delta(a + b, num_bits_fract);

	return (min + d);
}


unsigned int Decoder_LDPC_Binary_HW_Share::Calc_Modified_Systematic_Bits(unsigned int             iter,
                                                                         Buffer<int>             &input_bits_llr,
                                                                         Buffer<unsigned int, 2> &output_bits)
{
	unsigned int no_modified_systematic_bits = 0;

	// Calculate the number of modified signs of the systematic information.
	for(unsigned int i = 0; i < num_variable_nodes_ - num_check_nodes_; i++)
	{
		/*
		 * If input was 1 (negative LLR) and output is 0 or
		 * if input was 0 (positive LLR) and output is 1 then
		 * increment number of modified systematic bits.
		 */
		if((output_bits[iter - 1][i] == 0 && input_bits_llr[i]  < 0) ||
		   (output_bits[iter - 1][i] == 1 && input_bits_llr[i] >= 0))
		{
			no_modified_systematic_bits++;
		}
	}

	return no_modified_systematic_bits;
}

unsigned int Decoder_LDPC_Binary_HW_Share::Calc_Flipped_Bits(unsigned int iter, Buffer<unsigned int, 2> &output_bits)
{
    unsigned int flipped_bits = 0;
    // Calculate the number of flipped (sign) bits between iterations
    for (unsigned int i = 0; i != num_variable_nodes_; i++) {
        /*
         * If sign bit in current iteration is different from sign bit from previous
         * then increment number of flipped bits
         */
        if (output_bits[iter - 1][i] != output_bits[iter][i]) {
            flipped_bits++;
        }
    }
    return flipped_bits;
}

}
