//
//  Copyright (C) 2010 - 2012 Creonic GmbH
//
//  This file is part of the Creonic simulation environment (CSE)
//  for communication systems.
//
/// \file
/// \brief  Hardware-compliant LDPC decoder module for the IEEE 802.11ad standard.
/// \author Markus Fehrenz
/// \date   2012/01/24
//

#ifndef DEC_LDPC_IEEE_802_11AD_H_
#define DEC_LDPC_IEEE_802_11AD_H_

#include "dec_ldpc_bin_hw_share.h"
#include "dec_ldpc_bin_hw_iface.h"
#include "dec_ldpc_ieee_802_11ad_param.h"


namespace cse_lib {

/// LDPC decoder module for the IEEE_802_11ad Standard (N = 672, P = 42, R = (0.5 | 0.75 | 0.81) ).
/**
 * \ingroup modules
 */
class Decoder_LDPC_IEEE_802_11ad : public Decoder_LDPC_Binary_HW_Interface,
                                   public Decoder_LDPC_IEEE_802_11ad_Parameter,
                                   public Decoder_LDPC_Binary_HW_Share
{

public:

	Decoder_LDPC_IEEE_802_11ad() { };
	virtual ~Decoder_LDPC_IEEE_802_11ad() { };

	int Run();

private:

	Buffer<int, 2> app_ram_;  ///< APP RAM of LDPC decoder
	Buffer<int, 2> msg_ram_;  ///< Extrinsic RAM of LDPC Decoder

	void Init();


	/// Set decoder and code parameters
	/**
	 * Sets default decoder parameters to:
	 * - Layered decoding
	 * - Min-Sum algorithm with extrinsic scaling factor (0.875)
	 * - 5 iterations
	 * - 6 bits for extrinsics
	 * - 6 bits for channel values
	 * - 8 bits for APP values
	 */
	void Set_LDPC_Parameters();

};
}
#endif // DEC_LDPC_IEEE_802_11AD_H_
