#ifndef _BALLISTIX_WORKING_STUFF_H_
#define _BALLISTIX_WORKING_STUFF_H_

#include "trajectory_solver_API.h"
#include "trajectory_solver.h"
#include "nlohmann.h"

/********************************************************************************************

0.0.6.2 - Aerojump corrected
0.0.6.3 - Float replaced by doubles to reduce rounding error

********************************************************************************************/

#include <vector>
#include <string>

/*******************************************************************************************/

#define BALLISTIX_WORKING_BUFFER_SIZE 0x10000

/*******************************************************************************************/
namespace s2 {
	
	void solveBallistics(const std::string& inputJson, std::string& workBuffer);	
}
/*******************************************************************************************/

namespace s2 {

	class datapreparator {

		private:

			const char* version = "0.0.6.3";

			std::string m_token;
			
			bool m_makeRangecard{false};
			bool m_unitsIsMrads{false};
			windDataArray m_windArray{};
			
			std::vector<float> m_distances, m_verticals, m_horizontals, m_derivations, m_times;

			void prepareRangecardData(const Results& results);

		public:
			datapreparator() = default;
			Bullet parseForBulletData(const nlohmann::json& bodyJson) const;
			Rifle parseForRifleData(const nlohmann::json& bodyJson) const;
			Scope parseForScopeData(const nlohmann::json& bodyJson);
			Meteo parseForMeteoData(const nlohmann::json& bodyJson);
			Options parseForOptions(const nlohmann::json& bodyJson);
			Inputs parseForInputs(const nlohmann::json& bodyJson) const;
			void serializeResult(const Results& results, std::string& workBuffer);

			void getToken(const nlohmann::json& bodyJson);
	};
}

#endif /* _BALLISTIX_WORKING_STUFF_H_ */
